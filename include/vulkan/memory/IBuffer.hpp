/**
 * @file IBuffer.hpp
 * @brief Buffer 资源接口
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <span>
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief Buffer 接口
     * 
     * 抽象的 GPU Buffer 资源，支持不同的实现
     */
    class IBuffer
    {
        public:
            virtual ~IBuffer() = default;

            // 禁止拷贝
            IBuffer(const IBuffer&)            = delete;
            IBuffer& operator=(const IBuffer&) = delete;

            // 句柄访问
            [[nodiscard]] virtual VkBuffer handle() const noexcept = 0;
            [[nodiscard]] virtual bool     isValid() const noexcept = 0;

            // 属性访问
            [[nodiscard]] virtual uint64_t size() const noexcept = 0;
            [[nodiscard]] virtual uint32_t usage() const noexcept = 0; // VkBufferUsageFlags

            // 映射访问
            [[nodiscard]] virtual bool  isMapped() const noexcept = 0;
            [[nodiscard]] virtual void* mappedData() const noexcept = 0;

            // 数据操作
            virtual void* map() = 0;
            virtual void  unmap() = 0;

            // 便捷写入
            virtual void write(const void* data, uint64_t size, uint64_t offset = 0) = 0;
            virtual void write(const std::span<const std::byte>& data, uint64_t offset = 0) = 0;

            template <typename T> void writeT(const T& data, uint64_t offset = 0)
            {
                write(&data, sizeof(T), offset);
            }

            // 数据读取
            virtual void read(void* data, uint64_t size, uint64_t offset = 0) = 0;

            template <typename T> T readT(uint64_t offset = 0)
            {
                T data;
                read(&data, sizeof(T), offset);
                return data;
            }

            // 拷贝数据
            virtual void copyFrom(const IBuffer& source, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) = 0;

            // 刷新/使无效（非相干内存）
            virtual void flush(uint64_t offset = 0, uint64_t size = VK_WHOLE_SIZE) = 0;
            virtual void invalidate(uint64_t offset = 0, uint64_t size = VK_WHOLE_SIZE) = 0;

            // 获取设备地址（用于光线追踪/着色器访问）
            [[nodiscard]] virtual uint64_t deviceAddress() const = 0; // VkDeviceAddress

            // 获取分配信息
            [[nodiscard]] virtual const void* allocationInfo() const = 0; // 实现特定的分配信息
    };

    using IBufferPtr     = std::shared_ptr<IBuffer>;
    using IBufferWeakPtr = std::weak_ptr<IBuffer>;

    /**
     * @brief 类型安全的 Buffer 模板包装
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