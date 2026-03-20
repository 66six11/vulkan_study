#include "engine/rhi/vulkan/memory/Allocation.hpp"
#include "engine/core/utils/Logger.hpp"
#include <sstream>

namespace vulkan_engine::vulkan::memory
{
    Allocation::Allocation(std::shared_ptr<VmaAllocator> allocator, VmaAllocation allocation) noexcept
        : allocator_(allocator)
        , allocation_(allocation)
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            VmaAllocationInfo info;
            vmaGetAllocationInfo(allocator->handle(), allocation_, &info);
            mappedData_ = info.pMappedData;
            // 娉ㄦ剰锛氭寔涔呮槧灏勪笉璁剧疆 explicitlyMapped_锛屽洜涓轰笉闇€瑕?vmaUnmapMemory
        }
    }

    Allocation::~Allocation()
    {
        cleanup();
    }

    Allocation::Allocation(Allocation&& other) noexcept
        : allocator_(std::move(other.allocator_))
        , allocation_(other.allocation_)
        , mappedData_(other.mappedData_)
        , explicitlyMapped_(other.explicitlyMapped_)
    {
        other.allocation_       = VK_NULL_HANDLE;
        other.mappedData_       = nullptr;
        other.explicitlyMapped_ = false;
    }

    Allocation& Allocation::operator=(Allocation&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            allocator_              = std::move(other.allocator_);
            allocation_             = other.allocation_;
            mappedData_             = other.mappedData_;
            explicitlyMapped_       = other.explicitlyMapped_;
            other.allocation_       = VK_NULL_HANDLE;
            other.mappedData_       = nullptr;
            other.explicitlyMapped_ = false;
        }
        return *this;
    }

    void Allocation::cleanup() noexcept
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                // 鍙湁鏄惧紡鏄犲皠鐨勬墠闇€瑕?vmaUnmapMemory锛屾寔涔呮槧灏勪笉闇€瑕?
                if (explicitlyMapped_ && mappedData_ != nullptr)
                {
                    vmaUnmapMemory(allocator->handle(), allocation_);
                }
                vmaFreeMemory(allocator->handle(), allocation_);
            }
            allocation_       = VK_NULL_HANDLE;
            mappedData_       = nullptr;
            explicitlyMapped_ = false;
        }
    }

    AllocationInfo Allocation::getInfo() const
    {
        AllocationInfo result = {};
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                VmaAllocationInfo info;
                vmaGetAllocationInfo(allocator->handle(), allocation_, &info);
                result.size            = info.size;
                result.memoryTypeIndex = info.memoryType;
                result.mappedData      = info.pMappedData;
            }
        }
        return result;
    }

    VkDeviceSize Allocation::size() const
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                VmaAllocationInfo info;
                vmaGetAllocationInfo(allocator->handle(), allocation_, &info);
                return info.size;
            }
        }
        return 0;
    }

    void* Allocation::map()
    {
        if (mappedData_ != nullptr)
        {
            return mappedData_;
        }

        if (allocation_ == VK_NULL_HANDLE)
        {
            return nullptr;
        }

        if (auto allocator = allocator_.lock())
        {
            VkResult result = vmaMapMemory(allocator->handle(), allocation_, &mappedData_);
            if (result != VK_SUCCESS)
            {
                std::ostringstream oss;
                oss << "Failed to map allocation: " << result;
                LOG_ERROR(oss.str());
                return nullptr;
            }
            // 鏍囪涓烘樉寮忔槧灏勶紝鏋愭瀯鏃堕渶瑕?vmaUnmapMemory
            explicitlyMapped_ = true;
        }
        return mappedData_;
    }

    void Allocation::unmap()
    {
        if (mappedData_ != nullptr && allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                vmaUnmapMemory(allocator->handle(), allocation_);
            }
            mappedData_ = nullptr;
        }
    }

    void Allocation::flush(VkDeviceSize offset, VkDeviceSize size)
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                vmaFlushAllocation(allocator->handle(), allocation_, offset, size);
            }
        }
    }

    void Allocation::invalidate(VkDeviceSize offset, VkDeviceSize size)
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                vmaInvalidateAllocation(allocator->handle(), allocation_, offset, size);
            }
        }
    }

    // AllocationBuilder 瀹炵幇
    AllocationBuilder& AllocationBuilder::hostVisible(bool persistentMap)
    {
        info_.usage = VMA_MEMORY_USAGE_AUTO;
        info_.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        info_.preferredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (persistentMap)
        {
            info_.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
        return *this;
    }

    AllocationBuilder& AllocationBuilder::deviceLocal()
    {
        info_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        info_.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::hostCached()
    {
        info_.preferredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::sequentialWrite()
    {
        info_.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::strategyMinMemory()
    {
        info_.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::strategyMinTime()
    {
        info_.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::mapped()
    {
        info_.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::pool(VmaPool pool)
    {
        info_.pool = pool;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::userData(void* data)
    {
        info_.pUserData = data;
        return *this;
    }

    AllocationBuilder& AllocationBuilder::priority(float priority)
    {
        // VMA 3.3.0+ 鏀寔鍐呭瓨浼樺厛绾?
        info_.priority = priority;
        return *this;
    }
} // namespace vulkan_engine::vulkan::memory