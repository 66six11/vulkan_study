/**
 * @file IBuffer.hpp
 * @brief Buffer 璧勬簮鎺ュ彛
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <span>
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief Buffer 鎺ュ彛
     * 
     * 鎶借薄鐨?GPU Buffer 璧勬簮锛屾敮鎸佷笉鍚岀殑瀹炵幇
     */
    class IBuffer
    {
        public:
            virtual ~IBuffer() = default;

            // 绂佹鎷疯礉
            IBuffer(const IBuffer&)            = delete;
            IBuffer& operator=(const IBuffer&) = delete;

            // 鍙ユ焺璁块棶
            [[nodiscard]] virtual VkBuffer handle() const noexcept = 0;
            [[nodiscard]] virtual bool     isValid() const noexcept = 0;

            // 灞炴€ц闂?
            [[nodiscard]] virtual uint64_t size() const noexcept = 0;
            [[nodiscard]] virtual uint32_t usage() const noexcept = 0; // VkBufferUsageFlags

            // 鏄犲皠璁块棶
            [[nodiscard]] virtual bool  isMapped() const noexcept = 0;
            [[nodiscard]] virtual void* mappedData() const noexcept = 0;

            // 鏁版嵁鎿嶄綔
            virtual void* map() = 0;
            virtual void  unmap() = 0;

            // 渚挎嵎鍐欏叆
            virtual void write(const void* data, uint64_t size, uint64_t offset = 0) = 0;
            virtual void write(const std::span<const std::byte>& data, uint64_t offset = 0) = 0;

            template <typename T> void writeT(const T& data, uint64_t offset = 0)
            {
                write(&data, sizeof(T), offset);
            }

            // 鏁版嵁璇诲彇
            virtual void read(void* data, uint64_t size, uint64_t offset = 0) = 0;

            template <typename T> T readT(uint64_t offset = 0)
            {
                T data;
                read(&data, sizeof(T), offset);
                return data;
            }

            // 鎷疯礉鏁版嵁
            virtual void copyFrom(const IBuffer& source, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) = 0;

            // 鍒锋柊/浣挎棤鏁堬紙闈炵浉骞插唴瀛橈級
            virtual void flush(uint64_t offset = 0, uint64_t size = VK_WHOLE_SIZE) = 0;
            virtual void invalidate(uint64_t offset = 0, uint64_t size = VK_WHOLE_SIZE) = 0;

            // 鑾峰彇璁惧鍦板潃锛堢敤浜庡厜绾胯拷韪?鐫€鑹插櫒璁块棶锛?
            [[nodiscard]] virtual uint64_t deviceAddress() const = 0; // VkDeviceAddress

            // 鑾峰彇鍒嗛厤淇℃伅
            [[nodiscard]] virtual const void* allocationInfo() const = 0; // 瀹炵幇鐗瑰畾鐨勫垎閰嶄俊鎭?
    };

    using IBufferPtr     = std::shared_ptr<IBuffer>;
    using IBufferWeakPtr = std::weak_ptr<IBuffer>;

    /**
     * @brief 绫诲瀷瀹夊叏鐨?Buffer 妯℃澘鍖呰
     */
    template <typename T> class TypedBuffer
    {
        public:
            explicit TypedBuffer(IBufferPtr buffer)
                : buffer_(std::move(buffer))
            {
            }

            [[nodiscard]] IBuffer*   get() const noexcept { return buffer_.get(); }
            [[nodiscard]] IBufferPtr shared() const noexcept { return buffer_; }
            [[nodiscard]] VkBuffer   handle() const noexcept { return buffer_ ? buffer_->handle() : VK_NULL_HANDLE; }
            [[nodiscard]] bool       isValid() const noexcept { return buffer_ && buffer_->isValid(); }

            [[nodiscard]] uint64_t elementCount() const noexcept
            {
                return buffer_ ? buffer_->size() / sizeof(T) : 0;
            }

            void writeElement(const T& data, uint64_t index)
            {
                if (buffer_)
                {
                    buffer_->write(&data, sizeof(T), index * sizeof(T));
                }
            }

            void writeElements(const T* data, uint64_t count, uint64_t startIndex = 0)
            {
                if (buffer_)
                {
                    buffer_->write(data, count * sizeof(T), startIndex * sizeof(T));
                }
            }

            [[nodiscard]] T readElement(uint64_t index)
            {
                T data{};
                if (buffer_)
                {
                    buffer_->read(&data, sizeof(T), index * sizeof(T));
                }
                return data;
            }

        private:
            IBufferPtr buffer_;
    };
} // namespace vulkan_engine::vulkan::memory