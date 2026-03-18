#pragma once

#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/memory/Allocation.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <span>
#include <optional>
#include <cstring>

namespace vulkan_engine::vulkan::memory
{
    // VMA Buffer 类
    class VmaBuffer
    {
        public:
            VmaBuffer(
                std::shared_ptr<VmaAllocator>  allocator,
                VkDeviceSize                   size,
                VkBufferUsageFlags             usage,
                const VmaAllocationCreateInfo& allocInfo);
            ~VmaBuffer();

            // Non-copyable
            VmaBuffer(const VmaBuffer&)            = delete;
            VmaBuffer& operator=(const VmaBuffer&) = delete;

            // Movable
            VmaBuffer(VmaBuffer&& other) noexcept;
            VmaBuffer& operator=(VmaBuffer&& other) noexcept;

            // 数据访问
            void* map();
            void  unmap();
            bool  isMapped() const noexcept { return mappedData_ != nullptr || allocationInfo_.isPersistentMapped; }

            // 便捷写入方法
            void write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
            void write(const std::span<const std::byte>& data, VkDeviceSize offset = 0);

            template <typename T> void writeT(const T& data, VkDeviceSize offset = 0)
            {
                write(&data, sizeof(T), offset);
            }

            // 读取数据
            void read(void* data, VkDeviceSize size, VkDeviceSize offset = 0);

            template <typename T> T readT(VkDeviceSize offset = 0)
            {
                T data;
                read(&data, sizeof(T), offset);
                return data;
            }

            // 拷贝数据
            void copyFrom(const VmaBuffer& source, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

            // 刷新/使无效（非相干内存）
            void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
            void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

            // 访问器
            VkBuffer           handle() const noexcept { return buffer_; }
            VkDeviceSize       size() const noexcept { return size_; }
            VkBufferUsageFlags usage() const noexcept { return usage_; }
            AllocationInfo     allocationInfo() const { return allocationInfo_; }
            bool               isValid() const noexcept { return buffer_ != VK_NULL_HANDLE && allocation_.isValid(); }

            // 获取分配
            const Allocation& allocation() const { return allocation_; }

            // 获取设备地址（用于光线追踪/着色器访问）
            VkDeviceAddress deviceAddress() const;

            // 获取对齐要求
            static VkDeviceSize getAlignmentRequirements(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size, VkBufferUsageFlags usage);

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VkBuffer                      buffer_ = VK_NULL_HANDLE;
            Allocation                    allocation_;
            VkDeviceSize                  size_  = 0;
            VkBufferUsageFlags            usage_ = 0;
            AllocationInfo                allocationInfo_{};
            void*                         mappedData_ = nullptr;

            void cleanup() noexcept;
    };

    using VmaBufferPtr = std::shared_ptr<VmaBuffer>;

    // Buffer 构建器
    class VmaBufferBuilder
    {
        public:
            explicit VmaBufferBuilder(std::shared_ptr<VmaAllocator> allocator);

            // 基本配置
            VmaBufferBuilder& size(VkDeviceSize size);
            VmaBufferBuilder& usage(VkBufferUsageFlags usage);

            // 内存属性配置
            VmaBufferBuilder& hostVisible(bool persistentMap = false);
            VmaBufferBuilder& hostCached();
            VmaBufferBuilder& deviceLocal();
            VmaBufferBuilder& sequentialWrite();
            VmaBufferBuilder& createMapped();

            // 高级选项
            VmaBufferBuilder& pool(VmaPool pool);
            VmaBufferBuilder& priority(float priority);
            VmaBufferBuilder& allocationFlags(VmaAllocationCreateFlags flags);
            VmaBufferBuilder& requiredFlags(VkMemoryPropertyFlags flags);
            VmaBufferBuilder& preferredFlags(VkMemoryPropertyFlags flags);

            // 构建
            std::unique_ptr<VmaBuffer> build();
            VmaBufferPtr               buildShared();

            // 预设配置
            static std::unique_ptr<VmaBuffer> createStagingBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size);
            static std::unique_ptr<VmaBuffer> createVertexBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size);
            static std::unique_ptr<VmaBuffer> createIndexBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size);
            static std::unique_ptr<VmaBuffer> createUniformBuffer(
                std::shared_ptr<VmaAllocator> allocator,
                VkDeviceSize                  size,
                bool                          persistentMap = true);
            static std::unique_ptr<VmaBuffer> createStorageBuffer(
                std::shared_ptr<VmaAllocator> allocator,
                VkDeviceSize                  size,
                bool                          hostVisible = false);
            static std::unique_ptr<VmaBuffer> createIndirectBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size);

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VkDeviceSize                  size_  = 0;
            VkBufferUsageFlags            usage_ = 0;
            VmaAllocationCreateInfo       allocInfo_{};
            float                         priority_ = 0.5f;
    };

    // 每帧 Buffer 管理（用于 uniform buffer 等）
    template <typename T> class PerFrameBuffer
    {
        public:
            PerFrameBuffer(std::shared_ptr<VmaAllocator> allocator, uint32_t frameCount)
                : allocator_(std::move(allocator))
                , frameCount_(frameCount)
                , currentFrame_(0)
            {
                buffers_.reserve(frameCount);
                for (uint32_t i = 0; i < frameCount; ++i)
                {
                    buffers_.push_back(VmaBufferBuilder::createUniformBuffer(allocator_, sizeof(T), true));
                }
            }

            // 更新当前帧的数据
            void update(const T& data)
            {
                buffers_[currentFrame_]->writeT(data);
            }

            void update(uint32_t frameIndex, const T& data)
            {
                buffers_[frameIndex % frameCount_]->writeT(data);
            }

            // 获取当前帧的 buffer
            VmaBuffer* currentBuffer() const { return buffers_[currentFrame_].get(); }
            VkBuffer   currentHandle() const { return buffers_[currentFrame_]->handle(); }

            VmaBuffer* buffer(uint32_t frame) const { return buffers_[frame % frameCount_].get(); }
            VkBuffer   handle(uint32_t frame) const { return buffers_[frame % frameCount_]->handle(); }

            // 设置当前帧
            void     setFrame(uint32_t frame) { currentFrame_ = frame % frameCount_; }
            uint32_t currentFrame() const { return currentFrame_; }
            uint32_t frameCount() const { return frameCount_; }

        private:
            std::shared_ptr<VmaAllocator>           allocator_;
            std::vector<std::unique_ptr<VmaBuffer>> buffers_;
            uint32_t                                frameCount_;
            uint32_t                                currentFrame_;
    };
} // namespace vulkan_engine::vulkan::memory