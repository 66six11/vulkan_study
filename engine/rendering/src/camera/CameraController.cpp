#include "engine/rendering/camera/CameraController.hpp"

#include <imgui.h>

namespace vulkan_engine::rendering
{
    OrbitCameraController::OrbitCameraController(const Config& config)
        : config_(config)
    {
    }

    void OrbitCameraController::set_enabled(bool enabled)
    {
        // 褰撲粠鍚敤鍙樹负绂佺敤鏃讹紝閲嶇疆鎷栨嫿鐘舵€?
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

        // 娉ㄦ剰锛歩o.WantCaptureMouse 妫€鏌ュ凡绉婚櫎
        // 鐢辫皟鐢ㄨ€咃紙main.cpp锛夐€氳繃 is_viewport_content_hovered() 绮剧‘鎺у埗 enabled_ 鐘舵€?

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
            // 浣跨敤 ImGui 杈撳叆锛堥伩鍏嶄笌 ImGui 绐楀彛鍐茬獊锛?
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
            // 浣跨敤 InputManager
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
            // 鎷栨嫿妯″紡
            if (mouse_pressed)
            {
                if (!is_dragging_)
                {
                    // 寮€濮嬫嫋鎷?
                    is_dragging_  = true;
                    last_mouse_x_ = mouse_x;
                    last_mouse_y_ = mouse_y;
                }
                else
                {
                    // 姝ｅ湪鎷栨嫿
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
            // 鎮仠妯″紡锛堜笉闇€瑕佹嫋鎷斤級
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
            // 浣跨敤 ImGui 杈撳叆
            scroll = ImGui::GetIO().MouseWheel;
        }
        else
        {
            // 浣跨敤 InputManager
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