#pragma once

#include "rendering/resources/RenderTarget.hpp"
#include <memory>

// Forward declare ImTextureID
using ImTextureID = void*;

namespace vulkan_engine::rendering
{
    /**
     * @brief 视窗类
     * 管理视窗逻辑，不包括渲染资源管理
     *
     * 职责：
     * - 管理显示尺寸与渲染目标尺寸的关系
     * - 提供宽高比计算
     * - 提供 ImGui 纹理ID
     */
    class Viewport
    {
        public:
            Viewport();
            ~Viewport();

            // 禁止拷贝
            Viewport(const Viewport&)            = delete;
            Viewport& operator=(const Viewport&) = delete;

            // 允许移动
            Viewport(Viewport&& other) noexcept;
            Viewport& operator=(Viewport&& other) noexcept;

            // 初始化
            void initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<RenderTarget>          render_target);

            // 清理
            void cleanup();

            // 请求调整大小（延迟到下一帧）
            void request_resize(uint32_t width, uint32_t height);

            // 应用待处理的大小调整
            void apply_pending_resize();

            // 是否有待处理的大小调整
            bool is_resize_pending() const { return resize_pending_; }

            // 获取待处理的尺寸
            VkExtent2D pending_extent() const { return {pending_width_, pending_height_}; }

            // 立即调整大小
            void resize(uint32_t width, uint32_t height);

            // 获取渲染目标
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            // 尺寸信息
            uint32_t width() const { return display_width_; }
            uint32_t height() const { return display_height_; }
            float    aspect_ratio() const;

            // 兼容 SceneViewport 的 API
            VkExtent2D extent() const { return {display_width_, display_height_}; }
            VkExtent2D display_extent() const { return {display_width_, display_height_}; }

            // 渲染目标尺寸（可能与显示尺寸不同）
            uint32_t render_width() const { return render_target_ ? render_target_->width() : 0; }
            uint32_t render_height() const { return render_target_ ? render_target_->height() : 0; }

            // ImGui 纹理ID
            ImTextureID imgui_texture_id() const;

            // 检查是否需要重新创建 ImGui 描述符集
            bool needs_imgui_update() const { return imgui_needs_update_; }
            void mark_imgui_updated() { imgui_needs_update_ = false; }

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<RenderTarget>          render_target_;

            // 显示尺寸（逻辑尺寸）
            uint32_t display_width_  = 0;
            uint32_t display_height_ = 0;

            // 待处理的尺寸调整
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
            bool     resize_pending_ = false;

            // ImGui 资源
            mutable VkDescriptorSet imgui_descriptor_set_ = VK_NULL_HANDLE;
            mutable VkSampler       imgui_sampler_        = VK_NULL_HANDLE;
            mutable bool            imgui_needs_update_   = true;

            void create_imgui_sampler() const;
            void create_imgui_descriptor_set() const;
    };
} // namespace vulkan_engine::rendering