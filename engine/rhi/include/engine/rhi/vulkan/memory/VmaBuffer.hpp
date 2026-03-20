#pragma once

#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include "engine/rhi/vulkan/memory/Allocation.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <span>
#include <optional>
#include <cstring>

namespace vulkan_engine::vulkan::memory
{
    // VMA Buffer 绫?
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

            // 鏁版嵁璁块棶
            void* map();
            void  unmap();
            bool  isMapped() const noexcept { return mappedData_ != nullptr || allocationInfo_.isPersistentMapped; }

            // 渚挎嵎鍐欏叆鏂规硶
            void write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
            void write(const std::span<const std::byte>& data, VkDeviceSize offset = 0);

            template <typename T> void writeT(const T& data, VkDeviceSize offset = 0)
            {
                write(&data, sizeof(T), offset);
            }

            // 璇诲彇鏁版嵁
            void read(void* data, VkDeviceSize size, VkDeviceSize offset = 0);

            template <typename T> T readT(VkDeviceSize offset = 0)
            {
                T data;
                read(&data, sizeof(T), offset);
                return data;
            }

            // 鎷疯礉鏁版嵁
            void copyFrom(const VmaBuffer& source, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

            // 鍒锋柊/浣挎棤鏁堬紙闈炵浉骞插唴瀛橈級
            void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
            void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

            // 璁块棶鍣?
            VkBuffer           handle() const noexcept { return buffer_; }
            VkDeviceSize       size() const noexcept { return size_; }
            VkBufferUsageFlags usage() const noexcept { return usage_; }
            AllocationInfo     allocationInfo() const { return allocationInfo_; }
            bool               isValid() const noexcept { return buffer_ != VK_NULL_HANDLE && allocation_.isValid(); }

            // 鑾峰彇鍒嗛厤
            const Allocation& allocation() const { return allocation_; }

            // 鑾峰彇璁惧鍦板潃锛堢敤浜庡厜绾胯拷韪?鐫€鑹插櫒璁块棶锛?
            VkDeviceAddress deviceAddress() const;

            // 鑾峰彇瀵归綈瑕佹眰
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

    // Buffer 鏋勫缓鍣?
    class VmaBufferBuilder
    {
        public:
            explicit VmaBufferBuilder(std::shared_ptr<VmaAllocator> allocator);

            // 鍩烘湰閰嶇疆
            VmaBufferBuilder& size(VkDeviceSize size);
            VmaBufferBuilder& usage(VkBufferUsageFlags usage);

            // 鍐呭瓨灞炴€ч厤缃?
            VmaBufferBuilder& hostVisible(bool persistentMap = false);
            VmaBufferBuilder& hostCached();
            VmaBufferBuilder& deviceLocal();
            VmaBufferBuilder& sequentialWrite();
            VmaBufferBuilder& createMapped();

            // 楂樼骇閫夐」
            VmaBufferBuilder& pool(VmaPool pool);
            VmaBufferBuilder& priority(float priority);
            VmaBufferBuilder& allocationFlags(VmaAllocationCreateFlags flags);
            VmaBufferBuilder& requiredFlags(VkMemoryPropertyFlags flags);
            VmaBufferBuilder& preferredFlags(VkMemoryPropertyFlags flags);

            // 鏋勫缓
            std::unique_ptr<VmaBuffer> build();
            VmaBufferPtr               buildShared();

            // 棰勮閰嶇疆
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

    // 姣忓抚 Buffer 绠＄悊锛堢敤浜?uniform buffer 绛夛級
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

            // 鏇存柊褰撳墠甯х殑鏁版嵁
            void update(const T& data)
            {
                buffers_[currentFrame_]->writeT(data);
            }

            void update(uint32_t frameIndex, const T& data)
            {
                buffers_[frameIndex % frameCount_]->writeT(data);
            }

            // 鑾峰彇褰撳墠甯х殑 buffer
            VmaBuffer* currentBuffer() const { return buffers_[currentFrame_].get(); }
            VkBuffer   currentHandle() const { return buffers_[currentFrame_]->handle(); }

            VmaBuffer* buffer(uint32_t frame) const { return buffers_[frame % frameCount_].get(); }
            VkBuffer   handle(uint32_t frame) const { return buffers_[frame % frameCount_]->handle(); }

            // 璁剧疆褰撳墠甯?
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