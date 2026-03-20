#include "rendering/camera/CameraController.hpp"

#include <imgui.h>

namespace vulkan_engine::rendering
{
    OrbitCameraController::OrbitCameraController(const Config& config)
        : config_(config)
    {
    }

    void OrbitCameraController::set_enabled(bool enabled)
    {
        // 当从启用变为禁用时，重置拖拽状态
        if (is_enabled() && !enabled)
        {
            is_dragging_ = false;
        }
        CameraController::set_enabled(enabled);
    }

    void OrbitCameraController::update(float delta_time)
    {
        (void)delta_time;

        if (!enabled_)
            return;

        // 注意：io.WantCaptureMouse 检查已移除
        // 由调用者（main.cpp）通过 is_viewport_content_hovered() 精确控制 enabled_ 状态

        handle_rotation();
        handle_zoom();
    }

    void OrbitCameraController::handle_rotation()
    {
        auto camera = camera_.lock();
        if (!camera)
            return;

        bool  mouse_pressed = false;
        float mouse_x       = 0.0f, mouse_y = 0.0f;
        float delta_x       = 0.0f, delta_y = 0.0f;

        if (config_.use_imgui_input)
        {
            // 使用 ImGui 输入（避免与 ImGui 窗口冲突）
            ImGuiIO& io           = ImGui::GetIO();
            int      imgui_button = (config_.rotate_button == platform::MouseButton::Left)
                                        ? 0
                                        : (config_.rotate_button == platform::MouseButton::Right)
                                        ? 1
                                        : 2;
            mouse_pressed = ImGui::IsMouseDown(imgui_button);
            mouse_x       = io.MousePos.x;
            mouse_y       = io.MousePos.y;
            delta_x       = io.MouseDelta.x;
            delta_y       = io.MouseDelta.y;
        }
        else
        {
            // 使用 InputManager
            auto input = input_manager_.lock();
            if (!input)
                return;
            mouse_pressed = input->is_mouse_button_pressed(config_.rotate_button);
            auto [mx, my] = input->mouse_position();
            mouse_x       = static_cast<float>(mx);
            mouse_y       = static_cast<float>(my);
            auto [dx, dy] = input->mouse_delta();
            delta_x       = static_cast<float>(dx);
            delta_y       = static_cast<float>(dy);
        }

        if (config_.require_mouse_drag)
        {
            // 拖拽模式
            if (mouse_pressed)
            {
                if (!is_dragging_)
                {
                    // 开始拖拽
                    is_dragging_  = true;
                    last_mouse_x_ = mouse_x;
                    last_mouse_y_ = mouse_y;
                }
                else
                {
                    // 正在拖拽
                    float current_x = mouse_x;
                    float current_y = mouse_y;

                    float dx = current_x - last_mouse_x_;
                    float dy = current_y - last_mouse_y_;

                    if (dx != 0.0f || dy != 0.0f)
                    {
                        camera->on_mouse_drag(-dx, dy, config_.rotation_sensitivity);
                    }

                    last_mouse_x_ = current_x;
                    last_mouse_y_ = current_y;
                }
            }
            else
            {
                is_dragging_ = false;
            }
        }
        else
        {
            // 悬停模式（不需要拖拽）
            if (delta_x != 0.0f || delta_y != 0.0f)
            {
                camera->on_mouse_drag(delta_x, delta_y, config_.rotation_sensitivity);
            }
        }
    }

    void OrbitCameraController::handle_zoom()
    {
        auto camera = camera_.lock();
        if (!camera)
            return;

        float scroll = 0.0f;

        if (config_.use_imgui_input)
        {
            // 使用 ImGui 输入
            scroll = ImGui::GetIO().MouseWheel;
        }
        else
        {
            // 使用 InputManager
            auto input = input_manager_.lock();
            if (!input)
                return;
            scroll = static_cast<float>(input->scroll_delta());
        }

        if (scroll != 0.0f)
        {
            camera->on_mouse_scroll(scroll, config_.zoom_speed);
        }
    }
} // namespace vulkan_engine::rendering