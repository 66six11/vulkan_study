#include "vulkan/memory/Allocation.hpp"
#include "core/utils/Logger.hpp"
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
    {
        other.allocation_ = VK_NULL_HANDLE;
        other.mappedData_ = nullptr;
    }

    Allocation& Allocation::operator=(Allocation&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            allocator_        = std::move(other.allocator_);
            allocation_       = other.allocation_;
            mappedData_       = other.mappedData_;
            other.allocation_ = VK_NULL_HANDLE;
            other.mappedData_ = nullptr;
        }
        return *this;
    }

    void Allocation::cleanup() noexcept
    {
        if (allocation_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_.lock())
            {
                vmaFreeMemory(allocator->handle(), allocation_);
            }
            allocation_ = VK_NULL_HANDLE;
            mappedData_ = nullptr;
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

    // AllocationBuilder 实现
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
        // VMA 3.3.0+ 支持内存优先级
        info_.priority = priority;
        return *this;
    }
} // namespace vulkan_engine::vulkan::memory