#pragma once

// Compatibility layer: Depth buffer (uses Texture internally)

#include "engine/rhi/Device.hpp"
#include "engine/rhi/Texture.hpp"
#include <vulkan/vulkan.h>

namespace engine::vulkan
{
    // DepthBuffer wraps a depth texture with convenient interface
    class DepthBuffer
    {
        public:
            DepthBuffer() = default;

            DepthBuffer(std::shared_ptr<rhi::Device> device, uint32_t width, uint32_t height)
                : device_(std::move(device))
                , width_(width)
                , height_(height)
            {
                create();
            }

            ~DepthBuffer()
            {
                destroy();
            }

            // Non-copyable
            DepthBuffer(const DepthBuffer&)            = delete;
            DepthBuffer& operator=(const DepthBuffer&) = delete;

            // Movable
            DepthBuffer(DepthBuffer&& other) noexcept
                : device_(std::move(other.device_))
                , texture_(std::move(other.texture_))
                , view_(std::move(other.view_))
                , width_(other.width_)
                , height_(other.height_)
                , format_(other.format_)
            {
                other.width_  = 0;
                other.height_ = 0;
            }

            DepthBuffer& operator=(DepthBuffer&& other) noexcept
            {
                if (this != &other)
                {
                    destroy();
                    device_       = std::move(other.device_);
                    texture_      = std::move(other.texture_);
                    view_         = std::move(other.view_);
                    width_        = other.width_;
                    height_       = other.height_;
                    format_       = other.format_;
                    other.width_  = 0;
                    other.height_ = 0;
                }
                return *this;
            }

            void resize(uint32_t width, uint32_t height)
            {
                if (width_ == width && height_ == height) return;
                width_  = width;
                height_ = height;
                destroy();
                create();
            }

            [[nodiscard]] VkImage image() const
            {
                return texture_ ? texture_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] VkImageView image_view() const
            {
                return view_ ? view_->nativeHandle() : VK_NULL_HANDLE;
            }

            [[nodiscard]] VkFormat format() const { return format_; }
            [[nodiscard]] uint32_t width() const { return width_; }
            [[nodiscard]] uint32_t height() const { return height_; }

            [[nodiscard]] rhi::TextureHandle     texture() const { return texture_; }
            [[nodiscard]] rhi::TextureViewHandle view() const { return view_; }

        private:
            void create()
            {
                if (!device_ || width_ == 0 || height_ == 0) return;

                // Determine best depth format
                format_ = VK_FORMAT_D32_SFLOAT; // Default to D32

                rhi::TextureDesc desc{};
                desc.type         = rhi::TextureType::Texture2D;
                desc.format       = rhi::Format::D32_FLOAT;
                desc.extent       = {width_, height_, 1};
                desc.mipLevels    = 1;
                desc.arrayLayers  = 1;
                desc.samples      = rhi::SampleCount::Count1;
                desc.usage        = rhi::TextureUsage::DepthStencilAttachment;
                desc.initialState = rhi::ResourceState::Undefined;
                desc.debugName    = "DepthBuffer";

                auto texResult = device_->createTexture(desc);
                if (texResult.has_value())
                {
                    texture_ = texResult.value();

                    // Create view
                    rhi::TextureViewDesc viewDesc{};
                    viewDesc.type                             = rhi::TextureViewType::View2D;
                    viewDesc.format                           = rhi::Format::D32_FLOAT;
                    viewDesc.subresourceRange.aspectMask      = rhi::TextureAspect::Depth;
                    viewDesc.subresourceRange.baseMipLevel    = 0;
                    viewDesc.subresourceRange.mipLevelCount   = 1;
                    viewDesc.subresourceRange.baseArrayLayer  = 0;
                    viewDesc.subresourceRange.arrayLayerCount = 1;

                    auto viewResult = device_->createTextureView(texture_, viewDesc);
                    if (viewResult.has_value())
                    {
                        view_ = viewResult.value();
                    }
                }
            }

            void destroy()
            {
                view_.reset();
                texture_.reset();
            }

            std::shared_ptr<rhi::Device> device_;
            rhi::TextureHandle           texture_;
            rhi::TextureViewHandle       view_;
            uint32_t                     width_  = 0;
            uint32_t                     height_ = 0;
            VkFormat                     format_ = VK_FORMAT_D32_SFLOAT;
    };
} // namespace engine::vulkan