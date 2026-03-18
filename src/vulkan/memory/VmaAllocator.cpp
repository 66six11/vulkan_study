// 定义 VMA 实现宏（必须在一个 .cpp 文件中定义）
#define VMA_IMPLEMENTATION

#include "vulkan/memory/VmaAllocator.hpp"
#include "core/utils/Logger.hpp"
#include <cstring>
#include <sstream>

namespace vulkan_engine::vulkan::memory
{
    VmaAllocator::VmaAllocator(std::shared_ptr<DeviceManager> deviceManager, const CreateInfo& createInfo)
        : deviceManager_(std::move(deviceManager))
    {
        if (!deviceManager_)
        {
            throw std::runtime_error("VmaAllocator: DeviceManager is null");
        }

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice         = deviceManager_->physical_device().handle();
        allocatorInfo.device                 = deviceManager_->device().handle();
        allocatorInfo.instance               = deviceManager_->instance().handle();

        // 启用 VMA 标志
        allocatorInfo.flags = 0;
        if (createInfo.enableDefragmentation)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }
        if (createInfo.enableBudget)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }

        // 在 Debug 构建中启用内存泄漏检测
        #ifdef VULKAN_ENGINE_DEBUG
        if (createInfo.enableMemoryLeakDetection)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
        }
        #endif

        VmaVulkanFunctions vulkanFunctions                      = {};
        vulkanFunctions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
        vulkanFunctions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory                        = vkAllocateMemory;
        vulkanFunctions.vkFreeMemory                            = vkFreeMemory;
        vulkanFunctions.vkMapMemory                             = vkMapMemory;
        vulkanFunctions.vkUnmapMemory                           = vkUnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory                      = vkBindBufferMemory;
        vulkanFunctions.vkBindImageMemory                       = vkBindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer                          = vkCreateBuffer;
        vulkanFunctions.vkDestroyBuffer                         = vkDestroyBuffer;
        vulkanFunctions.vkCreateImage                           = vkCreateImage;
        vulkanFunctions.vkDestroyImage                          = vkDestroyImage;
        vulkanFunctions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
        vulkanFunctions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
        vulkanFunctions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
        vulkanFunctions.vkBindImageMemory2KHR                   = vkBindImageMemory2;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

        allocatorInfo.pVulkanFunctions = &vulkanFunctions;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create VMA allocator", __FILE__, __LINE__);
        }

        LOG_INFO("VMA Allocator created successfully");
    }

    VmaAllocator::~VmaAllocator()
    {
        cleanup();
    }

    VmaAllocator::VmaAllocator(VmaAllocator&& other) noexcept
        : deviceManager_(std::move(other.deviceManager_))
        , allocator_(other.allocator_)
        , pools_(std::move(other.pools_))
    {
        other.allocator_ = VK_NULL_HANDLE;
    }

    VmaAllocator& VmaAllocator::operator=(VmaAllocator&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            deviceManager_   = std::move(other.deviceManager_);
            allocator_       = other.allocator_;
            pools_           = std::move(other.pools_);
            other.allocator_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VmaAllocator::cleanup() noexcept
    {
        if (allocator_ != VK_NULL_HANDLE)
        {
            // 销毁所有池
            for (VmaPool pool : pools_)
            {
                vmaDestroyPool(allocator_, pool);
            }
            pools_.clear();

            vmaDestroyAllocator(allocator_);
            allocator_ = VK_NULL_HANDLE;
            LOG_INFO("VMA Allocator destroyed");
        }
    }

    VmaPool VmaAllocator::createPool(const PoolCreateInfo& info)
    {
        VmaPoolCreateInfo poolInfo = {};
        poolInfo.memoryTypeIndex   = info.memoryTypeIndex;
        poolInfo.blockSize         = info.blockSize;
        poolInfo.minBlockCount     = info.minBlockCount;
        poolInfo.maxBlockCount     = info.maxBlockCount;

        VmaPool  pool   = VK_NULL_HANDLE;
        VkResult result = vmaCreatePool(allocator_, &poolInfo, &pool);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create VMA memory pool: " + info.name, __FILE__, __LINE__);
        }

        pools_.push_back(pool);
        LOG_INFO("VMA Pool created: " + info.name);
        return pool;
    }

    void VmaAllocator::destroyPool(VmaPool pool)
    {
        auto it = std::find(pools_.begin(), pools_.end(), pool);
        if (it != pools_.end())
        {
            vmaDestroyPool(allocator_, pool);
            pools_.erase(it);
        }
    }

    VmaStats VmaAllocator::getStats() const
    {
        VmaStats result = {};

        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(allocator_, budgets);

        for (uint32_t i = 0; i < deviceManager_->memory_properties().memoryHeapCount; ++i)
        {
            result.totalBytesAllocated += budgets[i].statistics.allocationBytes;
            result.totalBytesUsed += budgets[i].statistics.blockBytes;
            result.allocationCount += static_cast<uint32_t>(budgets[i].statistics.allocationCount);
            result.blockCount += static_cast<uint32_t>(budgets[i].statistics.blockCount);
        }

        return result;
    }

    void VmaAllocator::printStats() const
    {
        VmaStats stats = getStats();

        std::ostringstream oss;
        oss << "=== VMA Statistics ===\n";
        oss << "  Total Allocated: " << stats.totalBytesAllocated / (1024.0 * 1024.0) << " MB\n";
        oss << "  Total Used: " << stats.totalBytesUsed / (1024.0 * 1024.0) << " MB\n";
        oss << "  Allocation Count: " << stats.allocationCount << "\n";
        oss << "  Block Count: " << stats.blockCount;
        LOG_INFO(oss.str());

        // 打印各堆预算
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(allocator_, budgets);

        for (uint32_t i = 0; i < deviceManager_->memory_properties().memoryHeapCount; ++i)
        {
            std::ostringstream heapOss;
            heapOss << "  Heap " << i << ": "
                    << budgets[i].usage / (1024.0 * 1024.0) << "/"
                    << budgets[i].budget / (1024.0 * 1024.0) << " MB ("
                    << ((budgets[i].budget > 0) ? (budgets[i].usage * 100 / budgets[i].budget) : 0) << "%)";
            LOG_INFO(heapOss.str());
        }
    }

    std::vector<VmaAllocator::Budget> VmaAllocator::getHeapBudgets() const
    {
        std::vector<Budget> result;
        VmaBudget           budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(allocator_, budgets);

        result.reserve(deviceManager_->memory_properties().memoryHeapCount);
        for (uint32_t i = 0; i < deviceManager_->memory_properties().memoryHeapCount; ++i)
        {
            result.push_back({budgets[i].budget, budgets[i].usage});
        }

        return result;
    }

    bool VmaAllocator::supportsMemoryType(uint32_t memoryTypeIndex, VkMemoryPropertyFlags requiredFlags) const
    {
        if (memoryTypeIndex >= deviceManager_->memory_properties().memoryTypeCount)
        {
            return false;
        }
        const auto& memType = deviceManager_->memory_properties().memoryTypes[memoryTypeIndex];
        return (memType.propertyFlags & requiredFlags) == requiredFlags;
    }

    std::string VmaAllocator::allocationFlagsToString(VmaAllocationCreateFlags flags)
    {
        std::string result;
        if (flags & VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
            result += "DEDICATED ";
        if (flags & VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT)
            result += "NEVER_ALLOCATE ";
        if (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
            result += "MAPPED ";
        if (flags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
            result += "SEQUENTIAL_WRITE ";
        if (flags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)
            result += "RANDOM_ACCESS ";
        if (flags & VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT)
            result += "MIN_MEMORY ";
        if (flags & VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT)
            result += "MIN_TIME ";
        return result.empty() ? "NONE" : result;
    }
} // namespace vulkan_engine::vulkan::memory