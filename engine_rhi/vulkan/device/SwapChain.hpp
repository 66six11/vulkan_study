#pragma once

// Compatibility layer: maps old vulkan::SwapChain to new engine::rhi::SwapChain

#include "engine/rhi/SwapChain.hpp"

namespace engine::vulkan
{
    // SwapChain compatibility - wraps the new RHI SwapChain with old interface
    class SwapChain
    {
        public:
            SwapChain() = default;

            explicit SwapChain(std::shared_ptr<rhi::SwapChain> impl)
                : impl_(std::move(impl))
            {
            }

            // Old interface methods mapping to new interface
            [[nodiscard]] uint32_t width() const
            {
                return impl_ ? impl_->extent().width : 0;
            }

            [[nodiscard]] uint32_t height() const
            {
                return impl_ ? impl_->extent().height : 0;
            }

            [[nodiscard]] uint32_t image_count() const
            {
                return impl_ ? impl_->imageCount() : 0;
            }

            [[nodiscard]] VkFormat format() const
            {
                // Map rhi::Format to VkFormat
                if (!impl_) return VK_FORMAT_UNDEFINED;
                // Note: This is a simplified mapping
                switch (impl_->format())
                {
                    case rhi::Format::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
                    case rhi::Format::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
                    case rhi::Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
                    case rhi::Format::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
                    default: return VK_FORMAT_UNDEFINED;
                }
            }

            [[nodiscard]] VkSwapchainKHR handle() const
            {
                return impl_ ? impl_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] uint32_t current_image_index() const
            {
                return impl_ ? impl_->currentImageIndex() : 0;
            }

            // Access to underlying RHI implementation
            [[nodiscard]] std::shared_ptr<rhi::SwapChain> rhi_swap_chain() const { return impl_; }

        private:
            std::shared_ptr<rhi::SwapChain> impl_;
    };
} // namespace engine::vulkan