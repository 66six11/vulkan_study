#pragma once

#include "engine/rendering/resources/RenderTarget.hpp"
#include <memory>

// Forward declare ImTextureID
using ImTextureID = void*;

namespace vulkan_engine::rendering
{
    /**
     * @brief 瑙嗙獥绫?
     * 绠＄悊瑙嗙獥閫昏緫锛屼笉鍖呮嫭娓叉煋璧勬簮绠＄悊
     *
     * 鑱岃矗锛?
     * - 绠＄悊鏄剧ず灏哄涓庢覆鏌撶洰鏍囧昂瀵哥殑鍏崇郴
     * - 鎻愪緵瀹介珮姣旇绠?
     * - 鎻愪緵瀵?RenderTarget 鐨勮闂紙鐢ㄤ簬 ImGui 绾圭悊鍒涘缓锛?
     * 
     * 娉ㄦ剰锛欼mGui 绾圭悊ID鐨勫垱寤哄凡绉昏嚦 ImGuiManager锛岄伩鍏?Viewport 渚濊禆 Vulkan 鍚庣缁嗚妭
     */
    class Viewport
    {
        public:
            Viewport();
            ~Viewport();

            // 绂佹鎷疯礉
            Viewport(const Viewport&)            = delete;
            Viewport& operator=(const Viewport&) = delete;

            // 鍏佽绉诲姩
            Viewport(Viewport&& other) noexcept;
            Viewport& operator=(Viewport&& other) noexcept;

            // 鍒濆鍖?
            void initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<RenderTarget>          render_target);

            // 娓呯悊
            void cleanup();

            // 璇锋眰璋冩暣澶у皬锛堝欢杩熷埌涓嬩竴甯э級
            void request_resize(uint32_t width, uint32_t height);

            // 搴旂敤寰呭鐞嗙殑澶у皬璋冩暣
            void apply_pending_resize();

            // 鏄惁鏈夊緟澶勭悊鐨勫ぇ灏忚皟鏁?
            bool is_resize_pending() const { return resize_pending_; }

            // 鑾峰彇寰呭鐞嗙殑灏哄
            VkExtent2D pending_extent() const { return {pending_width_, pending_height_}; }

            // 绔嬪嵆璋冩暣澶у皬
            void resize(uint32_t width, uint32_t height);

            // 鑾峰彇娓叉煋鐩爣
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            // 灏哄淇℃伅
            uint32_t width() const { return display_width_; }
            uint32_t height() const { return display_height_; }
            float    aspect_ratio() const;

            // 鍏煎 SceneViewport 鐨?API
            VkExtent2D extent() const { return {display_width_, display_height_}; }
            VkExtent2D display_extent() const { return {display_width_, display_height_}; }

            // 娓叉煋鐩爣灏哄锛堝彲鑳戒笌鏄剧ず灏哄涓嶅悓锛?
            uint32_t render_width() const { return render_target_ ? render_target_->width() : 0; }
            uint32_t render_height() const { return render_target_ ? render_target_->height() : 0; }

            // 鑾峰彇棰滆壊鍥惧儚瑙嗗浘锛堜緵 ImGuiManager 鍒涘缓绾圭悊锛?
            VkImageView color_image_view() const;

            // 鏍囪闇€瑕佹洿鏂?ImGui 绾圭悊
            void mark_imgui_texture_dirty() { imgui_texture_dirty_ = true; }
            bool is_imgui_texture_dirty() const { return imgui_texture_dirty_; }
            void clear_imgui_texture_dirty() { imgui_texture_dirty_ = false; }

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<RenderTarget>          render_target_;

            // 鏄剧ず灏哄锛堥€昏緫灏哄锛?
            uint32_t display_width_  = 0;
            uint32_t display_height_ = 0;

            // 寰呭鐞嗙殑灏哄璋冩暣
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
            bool     resize_pending_ = false;

            // ImGui 绾圭悊闇€瑕佹洿鏂版爣蹇?
            bool imgui_texture_dirty_ = true;
    };
} // namespace vulkan_engine::rendering