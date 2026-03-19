#include "rendering/Viewport.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include "core/utils/Logger.hpp"

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
        , imgui_texture_dirty_(other.imgui_texture_dirty_)
    {
    }

    Viewport& Viewport::operator=(Viewport&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            device_              = std::move(other.device_);
            render_target_       = std::move(other.render_target_);
            display_width_       = other.display_width_;
            display_height_      = other.display_height_;
            pending_width_       = other.pending_width_;
            pending_height_      = other.pending_height_;
            resize_pending_      = other.resize_pending_;
            imgui_texture_dirty_ = other.imgui_texture_dirty_;
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

        imgui_texture_dirty_ = true;

        logger::info("Viewport initialized: " + std::to_string(display_width_) + "x" + std::to_string(display_height_));
    }

    void Viewport::cleanup()
    {
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

        // 标记 ImGui 纹理需要更新
        imgui_texture_dirty_ = true;

        logger::info("Viewport resized to: " + std::to_string(width) + "x" + std::to_string(height));
    }

    float Viewport::aspect_ratio() const
    {
        if (display_height_ == 0)
            return 16.0f / 9.0f;
        return static_cast<float>(display_width_) / static_cast<float>(display_height_);
    }

    VkImageView Viewport::color_image_view() const
    {
        if (!render_target_)
            return VK_NULL_HANDLE;
        return render_target_->color_image_view();
    }
} // namespace vulkan_engine::rendering