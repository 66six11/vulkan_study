#pragma once

// Compatibility layer: VMA Image wrapper
// Note: Images are now created through engine::rhi::Device::createTexture

#include "engine/rhi/Texture.hpp"
#include "engine/rhi/Device.hpp"

namespace engine::vulkan::memory
{
    // VmaImage wraps an image allocated with VMA
    // This is largely obsolete with the new RHI
    class VmaImage
    {
        public:
            VmaImage()  = default;
            ~VmaImage() = default;

            explicit VmaImage(rhi::TextureHandle texture)
                : texture_(std::move(texture))
            {
            }

            [[nodiscard]] VkImage image() const
            {
                return texture_ ? texture_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] rhi::TextureHandle texture() const { return texture_; }

        private:
            rhi::TextureHandle texture_;
    };
} // namespace engine::vulkan::memory