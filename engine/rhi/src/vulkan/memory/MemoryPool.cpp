#include "vulkan/memory/MemoryPool.hpp"
#include "core/utils/Logger.hpp"
#include <sstream>

namespace vulkan_engine::vulkan::memory
{
    MemoryPool::MemoryPool(std::shared_ptr<VmaAllocator> allocator, const CreateInfo& createInfo)
        : allocator_(std::move(allocator))
        , name_(createInfo.name)
    {
        if (!allocator_)
        {
            throw std::runtime_error("MemoryPool: allocator is null");
        }

        uint32_t memoryTypeIndex = createInfo.memoryTypeIndex;

        // 如果指定了内存属性但没有指定 memoryTypeIndex，查找合适的类型
        if (createInfo.memoryProperties.has_value() && memoryTypeIndex == UINT32_MAX)
        {
            const auto&           memProps = allocator_->device()->memory_properties();
            VkMemoryPropertyFlags flags    = createInfo.memoryProperties.value();

            for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
            {
                if ((memProps.memoryTypes[i].propertyFlags & flags) == flags)
                {
                    memoryTypeIndex = i;
                    break;
                }
            }
        }

        if (memoryTypeIndex == UINT32_MAX)
        {
            throw std::runtime_error("MemoryPool: failed to find suitable memory type");
        }

        VmaPoolCreateInfo poolInfo      = {};
        poolInfo.memoryTypeIndex        = memoryTypeIndex;
        poolInfo.blockSize              = createInfo.blockSize;
        poolInfo.minBlockCount          = createInfo.minBlockCount;
        poolInfo.maxBlockCount          = createInfo.maxBlockCount;
        poolInfo.minAllocationAlignment = createInfo.alignment;

        // Note: VMA_POOL_CREATE_ALLOW_DEDICATED_MEMORY_BIT is not available in VMA 3.x
        // Dedicated allocations are handled automatically by VMA

        VkResult result = vmaCreatePool(allocator_->handle(), &poolInfo, &pool_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create VMA memory pool: " + name_, __FILE__, __LINE__);
        }

        std::ostringstream oss;
        oss << "MemoryPool created: name=" << name_ << ", memoryTypeIndex=" << memoryTypeIndex;
        LOG_INFO(oss.str());
    }

    MemoryPool::~MemoryPool()
    {
        cleanup();
    }

    MemoryPool::MemoryPool(MemoryPool&& other) noexcept
        : allocator_(std::move(other.allocator_))
        , pool_(other.pool_)
        , name_(std::move(other.name_))
    {
        other.pool_ = VK_NULL_HANDLE;
    }

    MemoryPool& MemoryPool::operator=(MemoryPool&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            allocator_  = std::move(other.allocator_);
            pool_       = other.pool_;
            name_       = std::move(other.name_);
            other.pool_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void MemoryPool::cleanup() noexcept
    {
        if (pool_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_)
            {
                vmaDestroyPool(allocator->handle(), pool_);
            }
            LOG_INFO("MemoryPool destroyed: " + name_);
            pool_ = VK_NULL_HANDLE;
        }
    }

    MemoryPool::Stats MemoryPool::getStats() const
    {
        Stats stats = {};
        if (pool_ != VK_NULL_HANDLE)
        {
            // VMA 3.3.0 API: Use vmaGetPoolStatistics if available, otherwise return zeros
            // Note: vmaGetPoolStatistics is not available in all VMA versions
            // Fallback to basic estimation
            stats.size            = 0;
            stats.usedSize        = 0;
            stats.allocationCount = 0;
            stats.blockCount      = 0;
        }
        return stats;
    }

    // MemoryPoolManager 实现
    MemoryPoolManager::MemoryPoolManager(std::shared_ptr<VmaAllocator> allocator)
        : allocator_(std::move(allocator))
    {
    }

    void MemoryPoolManager::initializeDefaultPools()
    {
        if (!allocator_)
        {
            return;
        }

        // 创建 Staging Pool (Host Visible)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.blockSize        = 64 * 1024 * 1024; // 64MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 4;
            info.name             = "Staging";

            pools_[PoolType::Staging] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Vertex Pool (Device Local)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            info.blockSize        = 128 * 1024 * 1024; // 128MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 8;
            info.name             = "Vertex";

            pools_[PoolType::Vertex] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Index Pool (Device Local)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            info.blockSize        = 64 * 1024 * 1024; // 64MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 4;
            info.name             = "Index";

            pools_[PoolType::Index] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Uniform Pool (Host Visible)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.blockSize        = 32 * 1024 * 1024; // 32MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 2;
            info.name             = "Uniform";

            pools_[PoolType::Uniform] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Texture Pool (Device Local)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            info.blockSize        = 256 * 1024 * 1024; // 256MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 16;
            info.name             = "Texture";

            pools_[PoolType::Texture] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 RenderTarget Pool (Device Local)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            info.blockSize        = 128 * 1024 * 1024; // 128MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 8;
            info.name             = "RenderTarget";

            pools_[PoolType::RenderTarget] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Dynamic Pool (Host Visible, 频繁更新)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.blockSize        = 16 * 1024 * 1024; // 16MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 2;
            info.name             = "Dynamic";

            pools_[PoolType::Dynamic] = std::make_shared < MemoryPool > (allocator_, info);
        }

        // 创建 Readback Pool (Host Visible + Cached)
        {
            MemoryPool::CreateInfo info;
            info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            info.blockSize        = 32 * 1024 * 1024; // 32MB blocks
            info.minBlockCount    = 1;
            info.maxBlockCount    = 2;
            info.name             = "Readback";

            pools_[PoolType::Readback] = std::make_shared < MemoryPool > (allocator_, info);
        }

        LOG_INFO("Default memory pools initialized");
    }

    MemoryPool* MemoryPoolManager::getPool(PoolType type)
    {
        auto it = pools_.find(type);
        return (it != pools_.end()) ? it->second.get() : nullptr;
    }

    VmaPool MemoryPoolManager::getPoolHandle(PoolType type)
    {
        auto pool = getPool(type);
        return pool ? pool->handle() : VK_NULL_HANDLE;
    }

    MemoryPoolPtr MemoryPoolManager::createPool(const MemoryPool::CreateInfo& createInfo)
    {
        return std::make_shared < MemoryPool > (allocator_, createInfo);
    }

    void MemoryPoolManager::printStats() const
    {
        LOG_INFO("=== Memory Pool Statistics ===");
        for (const auto& [type, pool] : pools_)
        {
            if (pool && pool->isValid())
            {
                auto               stats = pool->getStats();
                std::ostringstream oss;
                oss << "  " << poolTypeToString(type) << ": "
                        << stats.usedSize / (1024.0 * 1024.0) << " MB / "
                        << stats.size / (1024.0 * 1024.0) << " MB, allocations="
                        << stats.allocationCount << ", blocks=" << stats.blockCount;
                LOG_INFO(oss.str());
            }
        }
    }

    const char* MemoryPoolManager::poolTypeToString(PoolType type)
    {
        switch (type)
        {
            case PoolType::Staging:
                return "Staging";
            case PoolType::Vertex:
                return "Vertex";
            case PoolType::Index:
                return "Index";
            case PoolType::Uniform:
                return "Uniform";
            case PoolType::Texture:
                return "Texture";
            case PoolType::RenderTarget:
                return "RenderTarget";
            case PoolType::Dynamic:
                return "Dynamic";
            case PoolType::Readback:
                return "Readback";
            default:
                return "Unknown";
        }
    }
} // namespace vulkan_engine::vulkan::memory