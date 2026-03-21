#pragma once

// Compatibility layer: Uniform buffer template

#include "engine/rhi/Buffer.hpp"
#include "engine/rhi/Device.hpp"
#include <cstring>

namespace engine::vulkan
{
    // UniformBuffer - typed uniform buffer wrapper
    template <typename T> class UniformBuffer
    {
        public:
            UniformBuffer() = default;

            explicit UniformBuffer(std::shared_ptr<rhi::Device> device)
                : device_(std::move(device))
            {
                create();
            }

            ~UniformBuffer() = default;

            // Non-copyable
            UniformBuffer(const UniformBuffer&)            = delete;
            UniformBuffer& operator=(const UniformBuffer&) = delete;

            // Movable
            UniformBuffer(UniformBuffer&& other) noexcept
                : device_(std::move(other.device_))
                , buffer_(std::move(other.buffer_))
                , data_(std::move(other.data_))
            {
            }

            UniformBuffer& operator=(UniformBuffer&& other) noexcept
            {
                if (this != &other)
                {
                    device_ = std::move(other.device_);
                    buffer_ = std::move(other.buffer_);
                    data_   = std::move(other.data_);
                }
                return *this;
            }

            // Update the buffer data
            void update(const T& data)
            {
                data_ = data;
                if (buffer_)
                {
                    void* mapped = buffer_->mappedData();
                    if (mapped)
                    {
                        std::memcpy(mapped, &data_, sizeof(T));
                    }
                }
            }

            // Get current data
            [[nodiscard]] const T& data() const { return data_; }
            [[nodiscard]] T&       data() { return data_; }

            // Buffer access
            [[nodiscard]] rhi::BufferHandle buffer() const { return buffer_; }

            [[nodiscard]] VkBuffer handle() const
            {
                return buffer_ ? buffer_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] VkDeviceSize size() const { return sizeof(T); }

        private:
            void create()
            {
                if (!device_) return;

                rhi::BufferDesc desc{};
                desc.size           = sizeof(T);
                desc.usage          = rhi::BufferUsage::UniformBuffer;
                desc.memoryProperty = rhi::MemoryProperty::HostVisible | rhi::MemoryProperty::HostCoherent;
                desc.persistentMap  = true;
                desc.debugName      = "UniformBuffer";

                auto result = device_->createBuffer(desc);
                if (result.has_value())
                {
                    buffer_ = result.value();
                }
            }

            std::shared_ptr<rhi::Device> device_;
            rhi::BufferHandle            buffer_;
            T                            data_{};
    };
} // namespace engine::vulkan