#include <cstring>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "engine/rhi/Buffer.hpp"

namespace engine::rhi
{
    Buffer::~Buffer()
    {
        release();
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : handle_(other.handle_)
        , allocation_(other.allocation_)
        , allocator_(other.allocator_)
        , mappedData_(other.mappedData_)
        , size_(other.size_)
        , stride_(other.stride_)
        , usage_(other.usage_)
        , memoryProperties_(other.memoryProperties_)
        , persistentMap_(other.persistentMap_)
    {
        other.handle_     = nullptr;
        other.allocation_ = nullptr;
        other.allocator_  = nullptr;
        other.mappedData_ = nullptr;
        other.size_       = 0;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_           = other.handle_;
            allocation_       = other.allocation_;
            allocator_        = other.allocator_;
            mappedData_       = other.mappedData_;
            size_             = other.size_;
            stride_           = other.stride_;
            usage_            = other.usage_;
            memoryProperties_ = other.memoryProperties_;
            persistentMap_    = other.persistentMap_;
            other.handle_     = nullptr;
            other.allocation_ = nullptr;
            other.allocator_  = nullptr;
            other.mappedData_ = nullptr;
            other.size_       = 0;
        }
        return *this;
    }

    Buffer::Buffer(const InternalData& data, const BufferDesc& desc)
        : handle_(data.buffer)
        , allocation_(data.allocation)
        , allocator_(data.allocator)
        , mappedData_(data.mappedPtr)
        , size_(desc.size)
        , stride_(desc.stride)
        , usage_(desc.usage)
        , memoryProperties_(desc.memoryProperties)
        , persistentMap_(desc.persistentMap)
    {
    }

    void Buffer::release()
    {
        if (handle_ && allocator_)
        {
            // Unmap if mapped (and not persistent)
            if (mappedData_ && !persistentMap_)
            {
                vmaUnmapMemory(allocator_, allocation_);
            }

            // Destroy buffer
            vmaDestroyBuffer(allocator_, handle_, allocation_);

            handle_     = nullptr;
            allocation_ = nullptr;
            allocator_  = nullptr;
            mappedData_ = nullptr;
            size_       = 0;
        }
    }

    ResultValue<void*> Buffer::map()
    {
        if (!handle_)
        {
            return std::unexpected(Result::Error_InvalidParameter);
        }

        if (mappedData_)
        {
            return mappedData_; // Already mapped
        }

        // Check if memory is host visible
        if (!hasFlag(memoryProperties_, MemoryProperty::HostVisible))
        {
            return std::unexpected(Result::Error_Unsupported);
        }

        void*    data   = nullptr;
        VkResult result = vmaMapMemory(allocator_, allocation_, &data);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        mappedData_ = data;
        return data;
    }

    void Buffer::unmap()
    {
        if (mappedData_ && !persistentMap_ && allocator_)
        {
            vmaUnmapMemory(allocator_, allocation_);
            mappedData_ = nullptr;
        }
    }

    Result Buffer::writeData(const void* data, uint32_t dataSize, uint32_t offset)
    {
        if (!data || dataSize == 0)
        {
            return Result::Error_InvalidParameter;
        }

        if (offset + dataSize > size_)
        {
            return Result::Error_InvalidParameter;
        }

        if (mappedData_)
        {
            // Direct memory copy
            std::memcpy(static_cast<char*>(mappedData_) + offset, data, dataSize);
            return Result::Success;
        }

        // Not mapped - need to map first
        auto mapResult = map();
        if (!mapResult)
        {
            return mapResult.error();
        }

        std::memcpy(static_cast<char*>(mapResult.value()) + offset, data, dataSize);

        if (!persistentMap_)
        {
            unmap();
        }

        return Result::Success;
    }
} // namespace engine::rhi
