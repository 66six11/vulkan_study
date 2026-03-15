#include "rendering/Viewport.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include "core/utils/Logger.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace vulkan_engine::rendering
{
    Viewport::Viewport() = default;

    Viewport::~Viewport()
    {
        cleanup();
    }

    Viewport::Viewport(Viewport&& other) noexcept
        : device_(std::move(other.device_))
        , render_target_(std::move(other.render_target_))
        , display_width_(other.display_width_)
        , display_height_(other.display_height_)
        , pending_width_(other.pending_width_)
        , pending_height_(other.pending_height_)
        , resize_pending_(other.resize_pending_)
        , imgui_descriptor_set_(other.imgui_descriptor_set_)
        , imgui_sampler_(other.imgui_sampler_)
        , imgui_needs_update_(other.imgui_needs_update_)
    {
        other.imgui_descriptor_set_ = VK_NULL_HANDLE;
        other.imgui_sampler_        = VK_NULL_HANDLE;
    }

    Viewport& Viewport::operator=(Viewport&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            device_               = std::move(other.device_);
            render_target_        = std::move(other.render_target_);
            display_width_        = other.display_width_;
            display_height_       = other.display_height_;
            pending_width_        = other.pending_width_;
            pending_height_       = other.pending_height_;
            resize_pending_       = other.resize_pending_;
            imgui_descriptor_set_ = other.imgui_descriptor_set_;
            imgui_sampler_        = other.imgui_sampler_;
            imgui_needs_update_   = other.imgui_needs_update_;

            other.imgui_descriptor_set_ = VK_NULL_HANDLE;
            other.imgui_sampler_        = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Viewport::initialize(
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<RenderTarget>          render_target)
    {
        device_        = device;
        render_target_ = render_target;

        if (render_target_)
        {
            display_width_  = render_target_->width();
            display_height_ = render_target_->height();
        }

        imgui_needs_update_ = true;

        logger::info("Viewport initialized: " + std::to_string(display_width_) + "x" + std::to_string(display_height_));
    }

    void Viewport::cleanup()
    {
        if (imgui_descriptor_set_ != VK_NULL_HANDLE)
        {
            // ImGui 描述符集由 ImGui 管理，不需要手动销毁
            imgui_descriptor_set_ = VK_NULL_HANDLE;
        }

        if (imgui_sampler_ != VK_NULL_HANDLE && device_)
        {
            vkDestroySampler(device_->device(), imgui_sampler_, nullptr);
            imgui_sampler_ = VK_NULL_HANDLE;
        }

        render_target_.reset();
        device_.reset();
    }

    void Viewport::request_resize(uint32_t width, uint32_t height)
    {
        // 更新显示尺寸
        display_width_  = width;
        display_height_ = height;

        if (render_target_ && (width != render_target_->width() || height != render_target_->height()))
        {
            pending_width_  = width;
            pending_height_ = height;
            resize_pending_ = true;
        }
    }

    void Viewport::apply_pending_resize()
    {
        if (!resize_pending_)
            return;

        resize(pending_width_, pending_height_);
        resize_pending_ = false;
    }

    void Viewport::resize(uint32_t width, uint32_t height)
    {
        display_width_  = width;
        display_height_ = height;

        if (render_target_)
        {
            render_target_->resize(width, height);
        }

        // 标记需要更新 ImGui 描述符集
        imgui_descriptor_set_ = VK_NULL_HANDLE;
        imgui_needs_update_   = true;

        logger::info("Viewport resized to: " + std::to_string(width) + "x" + std::to_string(height));
    }

    float Viewport::aspect_ratio() const
    {
        if (display_height_ == 0)
            return 16.0f / 9.0f;
        return static_cast<float>(display_width_) / static_cast<float>(display_height_);
    }

    ImTextureID Viewport::imgui_texture_id() const
    {
        if (!render_target_)
            return nullptr;

        create_imgui_sampler();
        create_imgui_descriptor_set();

        return reinterpret_cast<ImTextureID>(imgui_descriptor_set_);
    }

    void Viewport::create_imgui_sampler() const
    {
        if (imgui_sampler_ != VK_NULL_HANDLE || !device_)
            return;

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter               = VK_FILTER_LINEAR;
        sampler_info.minFilter               = VK_FILTER_LINEAR;
        sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable        = VK_FALSE;
        sampler_info.maxAnisotropy           = 1.0f;
        sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable           = VK_FALSE;
        sampler_info.mipLodBias              = 0.0f;
        sampler_info.minLod                  = 0.0f;
        sampler_info.maxLod                  = 1.0f;

        VK_CHECK(vkCreateSampler(device_->device(),
                                 &sampler_info,
                                 nullptr,
                                 const_cast<VkSampler*>(&imgui_sampler_)));
    }

    void Viewport::create_imgui_descriptor_set() const
    {
        if (imgui_descriptor_set_ != VK_NULL_HANDLE || !render_target_ || imgui_sampler_ == VK_NULL_HANDLE)
            return;

        const_cast<Viewport*>(this)->imgui_descriptor_set_ =
                ImGui_ImplVulkan_AddTexture(
                                            imgui_sampler_,
                                            render_target_->color_image_view(),
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
} // namespace vulkan_engine::rendering