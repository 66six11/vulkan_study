#pragma once

// Compatibility layer: CommandBuffer and CommandPool

#include "engine/rhi/CommandBuffer.hpp"
#include "engine/rhi/Device.hpp"
#include <vulkan/vulkan.h>

namespace engine::vulkan
{
    // RenderCommandBuffer wraps engine::rhi::CommandBuffer with old interface
    class RenderCommandBuffer
    {
        public:
            RenderCommandBuffer() = default;

            explicit RenderCommandBuffer(rhi::CommandBufferHandle impl)
                : impl_(std::move(impl))
            {
            }

            // Recording control
            void begin() { if (impl_) impl_->begin(); }
            void end() { if (impl_) impl_->end(); }
            void reset() { if (impl_) impl_->reset(); }

            // Native handle access
            [[nodiscard]] VkCommandBuffer handle() const
            {
                return impl_ ? impl_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] bool is_recording() const
            {
                return impl_ ? impl_->isRecording() : false;
            }

            // Access underlying RHI command buffer
            [[nodiscard]] rhi::CommandBufferHandle rhi_cmd() const { return impl_; }
            [[nodiscard]] rhi::CommandBuffer*      operator->() const { return impl_.get(); }

            // Implicit conversion for compatibility
            operator bool() const { return impl_ != nullptr; }

        private:
            rhi::CommandBufferHandle impl_;
    };

    // RenderCommandPool manages command buffer allocation
    class RenderCommandPool
    {
        public:
            RenderCommandPool() = default;

            RenderCommandPool(
                std::shared_ptr<rhi::Device> device,
                uint32_t                     queue_family_index,
                VkCommandPoolCreateFlags     flags = 0)
                : device_(std::move(device))
                , queueFamilyIndex_(queue_family_index)
                , flags_(flags)
            {
            }

            // Allocate a single command buffer
            RenderCommandBuffer allocate_command_buffer()
            {
                if (!device_) return {};
                auto result = device_->allocateCommandBuffer(rhi::QueueType::Graphics);
                if (result.has_value())
                {
                    return RenderCommandBuffer(result.value());
                }
                return {};
            }

            // Allocate multiple command buffers
            std::vector<RenderCommandBuffer> allocate_command_buffers(uint32_t count)
            {
                std::vector<RenderCommandBuffer> buffers;
                if (!device_) return buffers;

                auto handles = device_->allocateCommandBuffers(rhi::QueueType::Graphics, count);
                buffers.reserve(handles.size());
                for (auto& h : handles)
                {
                    buffers.emplace_back(std::move(h));
                }
                return buffers;
            }

            void reset()
            {
                if (device_)
                {
                    device_->resetCommandPool(rhi::QueueType::Graphics);
                }
            }

        private:
            std::shared_ptr<rhi::Device> device_;
            uint32_t                     queueFamilyIndex_ = 0;
            VkCommandPoolCreateFlags     flags_            = 0;
    };
} // namespace engine::vulkan