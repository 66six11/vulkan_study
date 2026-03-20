#pragma once

#include "engine/rhi/vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace vulkan_engine::vulkan
{
    class Buffer
    {
        public:
            Buffer(
                std::shared_ptr<DeviceManager> device,
                VkDeviceSize                   size,
                VkBufferUsageFlags             usage,
                VkMemoryPropertyFlags          properties);
            ~Buffer();

            // Non-copyable
            Buffer(const Buffer&)            = delete;
            Buffer& operator=(const Buffer&) = delete;

            // Movable
            Buffer(Buffer&& other) noexcept;
            Buffer& operator=(Buffer&& other) noexcept;

            // Data access
            void* map();
            void  unmap();
            void  write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
            void  read(void* data, VkDeviceSize size, VkDeviceSize offset = 0);
            void  copy_from(
                const Buffer& source,
                VkDeviceSize  size,
                VkDeviceSize  src_offset = 0,
                VkDeviceSize  dst_offset = 0);

            // Flush and invalidate (for non-coherent memory)
            void flush(VkDeviceSize size, VkDeviceSize offset = 0);
            void invalidate(VkDeviceSize size, VkDeviceSize offset = 0);

            // Accessors
            VkBuffer       handle() const { return buffer_; }
            VkDeviceMemory memory() const { return memory_; }
            VkDeviceSize   size() const { return size_; }
            bool           is_mapped() const { return mapped_data_ != nullptr; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkBuffer                       buffer_      = VK_NULL_HANDLE;
            VkDeviceMemory                 memory_      = VK_NULL_HANDLE;
            VkDeviceSize                   size_        = 0;
            void*                          mapped_data_ = nullptr;
    };

    class BufferBuilder
    {
        public:
            explicit BufferBuilder(std::shared_ptr<DeviceManager> device);

            BufferBuilder& size(VkDeviceSize size);
            BufferBuilder& usage(VkBufferUsageFlags usage);
            BufferBuilder& host_visible(bool visible);
            BufferBuilder& host_cached(bool cached);
            BufferBuilder& device_local(bool local);

            std::unique_ptr<Buffer> build();

        private:
            std::shared_ptr<DeviceManager> device_;
            VkDeviceSize                   size_       = 0;
            VkBufferUsageFlags             usage_      = 0;
            VkMemoryPropertyFlags          properties_ = 0;
    };

    class BufferManager
    {
        public:
            struct Stats
            {
                uint64_t total_allocated = 0;
                uint64_t total_used      = 0;
                uint32_t buffer_count    = 0;
            };

            explicit BufferManager(std::shared_ptr<DeviceManager> device);
            ~BufferManager();

            std::shared_ptr<Buffer> create_buffer(
                VkDeviceSize          size,
                VkBufferUsageFlags    usage,
                VkMemoryPropertyFlags properties);
            std::shared_ptr<Buffer> create_vertex_buffer(VkDeviceSize size);
            std::shared_ptr<Buffer> create_index_buffer(VkDeviceSize size);
            std::shared_ptr<Buffer> create_uniform_buffer(VkDeviceSize size);
            std::shared_ptr<Buffer> create_staging_buffer(VkDeviceSize size);

            void destroy_buffer(std::shared_ptr<Buffer> buffer);

            Stats get_stats() const;

        private:
            std::shared_ptr<DeviceManager> device_;
    };
} // namespace vulkan_engine::vulkan