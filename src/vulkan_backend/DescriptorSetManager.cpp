#include "vulkan_backend/DescriptorSetManager.h"

#include <stdexcept>
#include <vector>


namespace
{
    std::vector<VkDescriptorPoolSize> buildPoolSizes(
        const DescriptorSetManager::PoolSizes& cfg,
        uint32_t                               maxSets)
    {
        std::vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(9);

        auto add = [&](VkDescriptorType type, float factor)
        {
            if (factor <= 0.0f)
            {
                return;
            }
            VkDescriptorPoolSize s{};
            s.type            = type;
            s.descriptorCount = static_cast<uint32_t>(factor * maxSets);
            if (s.descriptorCount == 0)
            {
                s.descriptorCount = 1; // 至少 1 个，避免创建时 descriptorCount 为 0
            }
            sizes.push_back(s);
        };

        add(VK_DESCRIPTOR_TYPE_SAMPLER, cfg.sampler);
        add(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, cfg.combinedImageSampler);
        add(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, cfg.sampledImage);
        add(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, cfg.storageImage);
        add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cfg.uniformBuffer);
        add(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, cfg.storageBuffer);
        add(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, cfg.uniformBufferDynamic);
        add(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, cfg.storageBufferDynamic);
        add(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, cfg.inputAttachment);

        return sizes;
    }
} // namespace

DescriptorSetManager::DescriptorSetManager(VulkanDevice& device)
    : device_(device)
{
}

DescriptorSetManager::~DescriptorSetManager()
{
    std::lock_guard lock(mutex_);
    for (auto& [layout, lp] : layoutPools_)
    {
        (void)layout;
        for (auto& pool : lp.pools)
        {
            if (pool.pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device_.device(), pool.pool, nullptr);
            }
        }
    }
}

VkDescriptorSet DescriptorSetManager::allocateSet(VkDescriptorSetLayout layout)
{
    auto sets = allocateSets(layout, 1);
    return sets.empty() ? VK_NULL_HANDLE : sets[0];
}

std::vector<VkDescriptorSet> DescriptorSetManager::allocateSets(
    VkDescriptorSetLayout layout,
    uint32_t              count)
{
    std::lock_guard lock(mutex_);

    std::vector<VkDescriptorSet> result(count, VK_NULL_HANDLE);
    uint32_t                     remaining = count;
    uint32_t                     offset    = 0;

    // 防御：避免潜在逻辑 bug 导致的死循环（理论上不会触发）
    const uint32_t kMaxPoolAttempts = 1024;
    uint32_t       attempts         = 0;

    while (remaining > 0)
    {
        if (++attempts > kMaxPoolAttempts)
        {
            throw std::runtime_error("DescriptorSetManager::allocateSets: too many pool attempts, possible logic bug.");
        }

        Pool& pool = getOrCreatePool(layout);

        uint32_t available = pool.maxSets - pool.usedSets;
        if (available == 0)
        {
            // 当前池已满，继续循环会在 getOrCreatePool 中创建新池
            continue;
        }

        uint32_t toAllocate = (remaining < available) ? remaining : available;

        std::vector<VkDescriptorSetLayout> layouts(toAllocate, layout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = pool.pool;
        allocInfo.descriptorSetCount = toAllocate;
        allocInfo.pSetLayouts        = layouts.data();

        VkResult res =
                vkAllocateDescriptorSets(device_.device(), &allocInfo, result.data() + offset);
        if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL)
        {
            // 当前池空间不足，标记为已满并创建新池再试
            pool.usedSets = pool.maxSets;
            continue;
        }
        else if (res != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets");
        }

        pool.usedSets += toAllocate;
        remaining -= toAllocate;
        offset += toAllocate;
    }

    return result;
}

void DescriptorSetManager::resetFrame()
{
    std::lock_guard lock(mutex_);
    for (auto& [layout, lp] : layoutPools_)
    {
        (void)layout;
        for (auto& pool : lp.pools)
        {
            if (pool.pool != VK_NULL_HANDLE)
            {
                vkResetDescriptorPool(device_.device(), pool.pool, 0);
                pool.usedSets = 0;
            }
        }
    }
}

void DescriptorSetManager::updateDescriptorSet(
    VkDescriptorSet                 set,
    std::span<VkWriteDescriptorSet> writes,
    std::span<VkCopyDescriptorSet>  copies) const
{
    if (!set)
    {
        return;
    }

    // 自动填充目标 set，避免调用端忘记设置 dstSet
    for (auto& w : writes)
    {
        w.dstSet = set;
    }

    // 对于拷贝操作，一般只需覆盖 dstSet；srcSet 通常由调用者决定来源
    for (auto& c : copies)
    {
        if (!c.dstSet)
        {
            c.dstSet = set;
        }
    }

    vkUpdateDescriptorSets(device_.device(),
                           static_cast<uint32_t>(writes.size()),
                           writes.data(),
                           static_cast<uint32_t>(copies.size()),
                           copies.data());
}

DescriptorSetManager::Pool& DescriptorSetManager::getOrCreatePool(VkDescriptorSetLayout layout)
{
    LayoutPools& lp = layoutPools_[layout];

    // 找到一个还有剩余空间的池
    for (auto& pool : lp.pools)
    {
        if (pool.usedSets < pool.maxSets)
        {
            return pool;
        }
    }

    // 没有可用池，创建一个新的
    // 可以根据需要提供配置接口或指数扩容，这里保持简单默认值
    uint32_t         newMaxSets = lp.pools.empty() ? 128u : (lp.pools.back().maxSets * 2u);
    VkDescriptorPool newPool    = createPool(defaultPoolSizes_, newMaxSets);

    Pool pool{};
    pool.pool     = newPool;
    pool.maxSets  = newMaxSets;
    pool.usedSets = 0;

    lp.pools.push_back(pool);
    return lp.pools.back();
}

VkDescriptorPool DescriptorSetManager::createPool(const PoolSizes& sizes, uint32_t maxSets) const
{
    std::vector<VkDescriptorPoolSize> poolSizes = buildPoolSizes(sizes, maxSets);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // 如需支持单独释放 descriptor set，可使用 VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    poolInfo.flags         = 0;
    poolInfo.maxSets       = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    return pool;
}
