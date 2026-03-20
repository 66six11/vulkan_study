#include "vulkan/resources/Buffer.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <cstring>

namespace vulkan_engine::vulkan
{
    Buffer::Buffer(
        std::shared_ptr<DeviceManager> device,
        VkDeviceSize                   size,
        VkBufferUsageFlags             usage,
        VkMemoryPropertyFlags          properties)
        : device_(std::move(device))
        , size_(size)
    {
        if (!device_)
        {
            throw std::runtime_error("Buffer: DeviceManager is null");
        }

        // Create buffer
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size        = size;
        buffer_info.usage       = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device_->device(), &buffer_info, nullptr, &buffer_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create buffer", __FILE__, __LINE__);
        }

        // RAII guard for exception safety
        struct BufferGuard
        {
            VkDevice  device;
            VkBuffer* buffer;

            ~BufferGuard()
            {
                if (buffer && *buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(device, *buffer, nullptr);
                }
            }
        } guard{device_->device(), &buffer_};

        // Get memory requirements
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device_->device(), buffer_, &mem_requirements);

        // Allocate memory
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_requirements.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(mem_requirements.memoryTypeBits, properties);

        result = vkAllocateMemory(device_->device(), &alloc_info, nullptr, &memory_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to allocate buffer memory", __FILE__, __LINE__);
        }

        // Bind memory to buffer
        result = vkBindBufferMemory(device_->device(), buffer_, memory_, 0);
        if (result != VK_SUCCESS)
        {
            vkFreeMemory(device_->device(), memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
            throw VulkanError(result, "Failed to bind buffer memory", __FILE__, __LINE__);
        }

        // Success - disable guard cleanup
        guard.buffer = nullptr;
    }

    Buffer::~Buffer()
    {
        if (buffer_ != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device_->device(), buffer_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(device_->device(), memory_, nullptr);
        }
    }

    void* Buffer::map()
    {
        void* data;
        vkMapMemory(device_->device(), memory_, 0, size_, 0, &data);
        return data;
    }

    void Buffer::unmap()
    {
        vkUnmapMemory(device_->device(), memory_);
    }

    void Buffer::write(const void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        void* mapped = map();
        memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));
        unmap();
    }

    void Buffer::read(void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        void* mapped = map();
        memcpy(data, static_cast<char*>(mapped) + offset, static_cast<size_t>(size));
        unmap();
    }

    void Buffer::copy_from(const Buffer& source, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset)
    {
        void* src_data = const_cast<Buffer&>(source).map();
        void* dst_data = map();
        memcpy(static_cast<char*>(dst_data) + dst_offset,
               static_cast<const char*>(src_data) + src_offset,
               static_cast<size_t>(size));
        const_cast<Buffer&>(source).unmap();
        unmap();
    }

    void Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange memory_range{};
        memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_range.memory = memory_;
        memory_range.offset = offset;
        memory_range.size   = size;
        vkFlushMappedMemoryRanges(device_->device(), 1, &memory_range);
    }

    void Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange memory_range{};
        memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memory_range.memory = memory_;
        memory_range.offset = offset;
        memory_range.size   = size;
        vkInvalidateMappedMemoryRanges(device_->device(), 1, &memory_range);
    }

    // BufferBuilder implementation
    BufferBuilder::BufferBuilder(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    BufferBuilder& BufferBuilder::size(VkDeviceSize size)
    {
        size_ = size;
        return *this;
    }

    BufferBuilder& BufferBuilder::usage(VkBufferUsageFlags usage)
    {
        usage_ = usage;
        return *this;
    }

    BufferBuilder& BufferBuilder::host_visible(bool visible)
    {
        if (visible)
        {
            properties_ |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
        else
        {
            properties_ &= ~(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
        return *this;
    }

    BufferBuilder& BufferBuilder::host_cached(bool cached)
    {
        if (cached)
        {
            properties_ |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        else
        {
            properties_ &= ~VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        return *this;
    }

    BufferBuilder& BufferBuilder::device_local(bool local)
    {
        if (local)
        {
            properties_ |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        else
        {
            properties_ &= ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        return *this;
    }

    std::unique_ptr<Buffer> BufferBuilder::build()
    {
        return std::make_unique < Buffer > (device_, size_, usage_, properties_);
    }

    // BufferManager implementation
    BufferManager::BufferManager(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    BufferManager::~BufferManager() = default;

    std::shared_ptr<Buffer> BufferManager::create_buffer(
        VkDeviceSize          size,
        VkBufferUsageFlags    usage,
        VkMemoryPropertyFlags properties)
    {
        return std::make_shared < Buffer > (device_, size, usage, properties);
    }

    std::shared_ptr<Buffer> BufferManager::create_vertex_buffer(VkDeviceSize size)
    {
        return create_buffer(size,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    std::shared_ptr<Buffer> BufferManager::create_index_buffer(VkDeviceSize size)
    {
        return create_buffer(size,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    std::shared_ptr<Buffer> BufferManager::create_uniform_buffer(VkDeviceSize size)
    {
        return create_buffer(size,
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    std::shared_ptr<Buffer> BufferManager::create_staging_buffer(VkDeviceSize size)
    {
        return create_buffer(size,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void BufferManager::destroy_buffer(std::shared_ptr<Buffer> buffer)
    {
        buffer.reset();
    }

    BufferManager::Stats BufferManager::get_stats() const
    {
        return Stats{};
    }
} // namespace vulkan_engine::vulkan