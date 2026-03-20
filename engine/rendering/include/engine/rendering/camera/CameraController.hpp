#pragma once

#include "engine/core/math/Camera.hpp"
#include "engine/platform/input/InputManager.hpp"
#include <memory>

// Forward declare ImGui IO
struct ImGuiIO;

namespace vulkan_engine::rendering
{
    // 鍓嶅悜澹版槑
    class Viewport;

    /**
     * @brief 鐩告満鎺у埗鍣ㄥ熀绫?
     * 瑙ｈ€﹁緭鍏ュ鐞嗕笌鐩告満閫昏緫
     */
    class CameraController
    {
        public:
            virtual ~CameraController() = default;

            /**
         * @brief 鏇存柊鎺у埗鍣ㄧ姸鎬?
         * @param delta_time 甯ф椂闂撮棿闅旓紙绉掞級
         */
            virtual void update(float delta_time) = 0;

            /**
         * @brief 璁剧疆鏄惁鍚敤
         */
            virtual void set_enabled(bool enabled) { enabled_ = enabled; }
            bool         is_enabled() const { return enabled_; }

            /**
         * @brief 闄勫姞鐩告満
         */
            void attach_camera(std::shared_ptr<core::OrbitCamera> camera) { camera_ = camera; }

            /**
         * @brief 闄勫姞杈撳叆绠＄悊鍣?
         */
            void attach_input_manager(std::shared_ptr<platform::InputManager> input_manager)
            {
                input_manager_ = input_manager;
            }

            /**
         * @brief 闄勫姞瑙嗙獥锛堢敤浜庤幏鍙栧楂樻瘮锛?
         */
            void attach_viewport(std::shared_ptr<Viewport> viewport) { viewport_ = viewport; }

        protected:
            std::weak_ptr<core::OrbitCamera>      camera_;
            std::weak_ptr<platform::InputManager> input_manager_;
            std::weak_ptr<Viewport>               viewport_;
            bool                                  enabled_ = true;
    };

    /**
     * @brief 杞ㄩ亾鐩告満鎺у埗鍣?
     * 澶勭悊榧犳爣鎷栨嫿鏃嬭浆鍜屾粴杞缉鏀?
     */
    class OrbitCameraController : public CameraController
    {
        public:
            struct Config
            {
                float                 rotation_sensitivity = 0.5f;                        // 鏃嬭浆鐏垫晱搴?
                float                 zoom_speed           = 0.1f;                        // 缂╂斁閫熷害
                bool                  require_mouse_drag   = true;                        // 鏄惁闇€瑕佹嫋鎷斤紙true=鎷栨嫿锛宖alse=鎮仠锛?
                platform::MouseButton rotate_button        = platform::MouseButton::Left; // 鏃嬭浆鎸夐敭
                bool                  use_imgui_input      = true;                        // 浣跨敤 ImGui 杈撳叆锛堥伩鍏嶄笌 ImGui 绐楀彛鍐茬獊锛?
            };

            explicit OrbitCameraController(const Config& config = {});
            ~OrbitCameraController() override = default;

            void update(float delta_time) override;

            // 閰嶇疆
            void          set_config(const Config& config) { config_ = config; }
            const Config& config() const { return config_; }

            // 妫€鏌ユ槸鍚︽鍦ㄦ嫋鎷?
            bool is_dragging() const { return is_dragging_; }

            // 璁剧疆鍚敤鐘舵€侊紙閲嶅啓浠ュ鐞嗘嫋鎷界姸鎬侀噸缃級
            void set_enabled(bool enabled) override;

        private:
            Config config_;
            bool   is_dragging_  = false;
            float  last_mouse_x_ = 0.0f;
            float  last_mouse_y_ = 0.0f;

            void handle_rotation();
            void handle_zoom();
    };
} // namespace vulkan_engine::rendering