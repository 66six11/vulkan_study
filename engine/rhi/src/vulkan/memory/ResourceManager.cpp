#include "engine/rhi/vulkan/memory/ResourceManager.hpp"
#include "engine/core/utils/Logger.hpp"

namespace vulkan_engine::vulkan::memory
{
    ResourceManager::ResourceManager(std::shared_ptr<DeviceManager> deviceManager, const CreateInfo& createInfo)
        : device_(std::move(deviceManager))
    {
        if (!device_)
        {
            throw std::runtime_error("ResourceManager: deviceManager is null");
        }

        // 鍒涘缓 VMA 鍒嗛厤鍣?
        VmaAllocator::CreateInfo allocatorInfo;
        allocatorInfo.enableDefragmentation     = createInfo.enableDefragmentation;
        allocatorInfo.enableBudget              = createInfo.enableBudget;
        allocatorInfo.enableMemoryLeakDetection = true;

        allocator_ = std::make_shared<VmaAllocator>(device_, allocatorInfo);

        // 鍒涘缓鍐呭瓨姹犵鐞嗗櫒
        poolManager_ = std::make_unique<MemoryPoolManager>(allocator_);

        if (createInfo.enableDefaultPools)
        {
            poolManager_->initializeDefaultPools();
        }

        LOG_INFO("ResourceManager created successfully");
    }

    VmaBufferPtr ResourceManager::createBuffer(
        VkDeviceSize                   size,
        VkBufferUsageFlags             usage,
        const VmaAllocationCreateInfo& allocInfo)
    {
        auto buffer            = std::make_shared<VmaBuffer>(allocator_, size, usage, allocInfo);
        buffers_[buffer.get()] = buffer;
        return buffer;
    }

    VmaBufferPtr ResourceManager::createStagingBuffer(VkDeviceSize size)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        // 浣跨敤 staging pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::Staging))
        {
            allocInfo.pool = pool;
        }

        return createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo);
    }

    VmaBufferPtr ResourceManager::createVertexBuffer(VkDeviceSize size)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        // 浣跨敤 vertex pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::Vertex))
        {
            allocInfo.pool = pool;
        }

        return createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo);
    }

    VmaBufferPtr ResourceManager::createIndexBuffer(VkDeviceSize size)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        // 浣跨敤 index pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::Index))
        {
            allocInfo.pool = pool;
        }

        return createBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo);
    }

    VmaBufferPtr ResourceManager::createUniformBuffer(VkDeviceSize size, bool persistentMap)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags          = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        if (persistentMap)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            // VMA 瑕佹眰锛氫娇鐢?MAPPED_BIT 鏃跺繀椤绘寚瀹?HOST_ACCESS 鏍囧織
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        // 浣跨敤 uniform pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::Uniform))
        {
            allocInfo.pool = pool;
        }

        return createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, allocInfo);
    }

    VmaBufferPtr ResourceManager::createStorageBuffer(VkDeviceSize size, bool hostVisible)
    {
        VmaAllocationCreateInfo allocInfo = {};

        if (hostVisible)
        {
            allocInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        else
        {
            allocInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        return createBuffer(size,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            allocInfo);
    }

    VmaImagePtr ResourceManager::createImage(const VkImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo)
    {
        auto image           = std::make_shared<VmaImage>(allocator_, imageInfo, allocInfo);
        images_[image.get()] = image;
        return image;
    }

    VmaImagePtr ResourceManager::createColorAttachment(
        uint32_t              width,
        uint32_t              height,
        VkFormat              format,
        uint32_t              mipLevels,
        VkSampleCountFlagBits samples)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        // 浣跨敤 render target pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::RenderTarget))
        {
            allocInfo.pool = pool;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {width, height, 1};
        imageInfo.mipLevels         = mipLevels;
        imageInfo.arrayLayers       = 1;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples           = samples;
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        auto image           = std::make_shared<VmaImage>(allocator_, imageInfo, allocInfo);
        images_[image.get()] = image;
        return image;
    }

    VmaImagePtr ResourceManager::createDepthAttachment(
        uint32_t              width,
        uint32_t              height,
        VkFormat              format,
        VkSampleCountFlagBits samples)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        // 浣跨敤 render target pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::RenderTarget))
        {
            allocInfo.pool = pool;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {width, height, 1};
        imageInfo.mipLevels         = 1;
        imageInfo.arrayLayers       = 1;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples           = samples;
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        auto image           = std::make_shared<VmaImage>(allocator_, imageInfo, allocInfo);
        images_[image.get()] = image;
        return image;
    }

    VmaImagePtr ResourceManager::createTexture(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        uint32_t mipLevels,
        uint32_t arrayLayers)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        // 浣跨敤 texture pool锛堝鏋滃彲鐢級
        if (auto pool = poolManager_->getPoolHandle(PoolType::Texture))
        {
            allocInfo.pool = pool;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {width, height, 1};
        imageInfo.mipLevels         = mipLevels;
        imageInfo.arrayLayers       = arrayLayers;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        auto image           = std::make_shared<VmaImage>(allocator_, imageInfo, allocInfo);
        images_[image.get()] = image;
        return image;
    }

    VmaImagePtr ResourceManager::createCubemap(uint32_t size, VkFormat format, uint32_t mipLevels)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {size, size, 1};
        imageInfo.mipLevels         = mipLevels;
        imageInfo.arrayLayers       = 6;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        auto image           = std::make_shared<VmaImage>(allocator_, imageInfo, allocInfo);
        images_[image.get()] = image;
        return image;
    }

    void ResourceManager::printStats() const
    {
        allocator_->printStats();
        poolManager_->printStats();
    }

    std::string ResourceManager::buildStatsString(bool detailed) const
    {
        return allocator_->buildStatsString(detailed);
    }

    std::vector<VmaBudget> ResourceManager::getHeapBudgets() const
    {
        return allocator_->getHeapBudgets();
    }

    bool ResourceManager::isMemoryAvailable(VkDeviceSize requiredBytes) const
    {
        auto budgets = getHeapBudgets();
        for (const auto& budget : budgets)
        {
            if (budget.budget - budget.usage >= requiredBytes)
            {
                return true;
            }
        }
        return false;
    }

    void ResourceManager::destroyBuffer(VmaBufferPtr buffer)
    {
        if (buffer)
        {
            buffers_.erase(buffer.get());
            buffer.reset();
        }
    }

    void ResourceManager::destroyImage(VmaImagePtr image)
    {
        if (image)
        {
            images_.erase(image.get());
            image.reset();
        }
    }

    void ResourceManager::defragment()
    {
        // VMA 3.3.0+ 鏀寔纰庣墖鏁寸悊
        // 杩欐槸涓€涓崰浣嶇瀹炵幇锛屽畬鏁寸殑瀹炵幇闇€瑕佷娇鐢?vmaDefragmentationBegin/vmaDefragmentationEnd
        LOG_INFO("ResourceManager::defragment called (placeholder)");
    }

    void ResourceManager::flush()
    {
        // 寮哄埗閲婃斁鏈娇鐢ㄧ殑鍐呭瓨鍧?
        if (allocator_)
        {
            vmaSetCurrentFrameIndex(allocator_->handle(), 0);
        }
        LOG_INFO("ResourceManager::flush called");
    }
} // namespace vulkan_engine::vulkan::memory