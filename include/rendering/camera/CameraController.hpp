#pragma once

#include "core/math/Camera.hpp"
#include "platform/input/InputManager.hpp"
#include <memory>

namespace vulkan_engine::rendering
{
    // 前向声明
    class Viewport;

    /**
     * @brief 相机控制器基类
     * 解耦输入处理与相机逻辑
     */
    class CameraController
    {
        public:
            virtual ~CameraController() = default;

            /**
         * @brief 更新控制器状态
         * @param delta_time 帧时间间隔（秒）
         */
            virtual void update(float delta_time) = 0;

            /**
         * @brief 设置是否启用
         */
            void set_enabled(bool enabled) { enabled_ = enabled; }
            bool is_enabled() const { return enabled_; }

            /**
         * @brief 附加相机
         */
            void attach_camera(std::shared_ptr<core::OrbitCamera> camera) { camera_ = camera; }

            /**
         * @brief 附加输入管理器
         */
            void attach_input_manager(std::shared_ptr<platform::InputManager> input_manager)
            {
                input_manager_ = input_manager;
            }

            /**
         * @brief 附加视窗（用于获取宽高比）
         */
            void attach_viewport(std::shared_ptr<Viewport> viewport) { viewport_ = viewport; }

        protected:
            std::weak_ptr<core::OrbitCamera>      camera_;
            std::weak_ptr<platform::InputManager> input_manager_;
            std::weak_ptr<Viewport>               viewport_;
            bool                                  enabled_ = true;
    };

    /**
     * @brief 轨道相机控制器
     * 处理鼠标拖拽旋转和滚轮缩放
     */
    class OrbitCameraController : public CameraController
    {
        public:
            struct Config
            {
                float                 rotation_sensitivity = 0.5f;                        // 旋转灵敏度
                float                 zoom_speed           = 0.1f;                        // 缩放速度
                bool                  require_mouse_drag   = true;                        // 是否需要拖拽（true=拖拽，false=悬停）
                platform::MouseButton rotate_button        = platform::MouseButton::Left; // 旋转按键
            };

            explicit OrbitCameraController(const Config& config = {});
            ~OrbitCameraController() override = default;

            void update(float delta_time) override;

            // 配置
            void          set_config(const Config& config) { config_ = config; }
            const Config& config() const { return config_; }

            // 检查是否正在拖拽
            bool is_dragging() const { return is_dragging_; }

        private:
            Config config_;
            bool   is_dragging_  = false;
            float  last_mouse_x_ = 0.0f;
            float  last_mouse_y_ = 0.0f;

            void handle_rotation();
            void handle_zoom();
    };
} // namespace vulkan_engine::rendering