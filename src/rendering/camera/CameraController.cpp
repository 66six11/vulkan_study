#include "rendering/camera/CameraController.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    OrbitCameraController::OrbitCameraController(const Config& config)
        : config_(config)
    {
    }

    void OrbitCameraController::update(float delta_time)
    {
        (void)delta_time;

        if (!enabled_)
            return;

        auto input = input_manager_.lock();
        if (!input)
            return;

        handle_rotation();
        handle_zoom();
    }

    void OrbitCameraController::handle_rotation()
    {
        auto input  = input_manager_.lock();
        auto camera = camera_.lock();
        if (!input || !camera)
            return;

        if (config_.require_mouse_drag)
        {
            // 拖拽模式
            if (input->is_mouse_button_pressed(config_.rotate_button))
            {
                if (!is_dragging_)
                {
                    // 开始拖拽
                    is_dragging_            = true;
                    auto [mouse_x, mouse_y] = input->mouse_position();
                    last_mouse_x_           = static_cast<float>(mouse_x);
                    last_mouse_y_           = static_cast<float>(mouse_y);
                }
                else
                {
                    // 正在拖拽
                    auto  [mouse_x, mouse_y] = input->mouse_position();
                    float current_x          = static_cast<float>(mouse_x);
                    float current_y          = static_cast<float>(mouse_y);

                    float delta_x = current_x - last_mouse_x_;
                    float delta_y = current_y - last_mouse_y_;

                    if (delta_x != 0.0f || delta_y != 0.0f)
                    {
                        camera->on_mouse_drag(-delta_x, delta_y, config_.rotation_sensitivity);
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
            auto [delta_x, delta_y] = input->mouse_delta();
            if (delta_x != 0.0 || delta_y != 0.0)
            {
                camera->on_mouse_drag(static_cast<float>(delta_x),
                                      static_cast<float>(delta_y),
                                      config_.rotation_sensitivity);
            }
        }
    }

    void OrbitCameraController::handle_zoom()
    {
        auto input  = input_manager_.lock();
        auto camera = camera_.lock();
        if (!input || !camera)
            return;

        double scroll = input->scroll_delta();
        if (scroll != 0.0)
        {
            camera->on_mouse_scroll(static_cast<float>(scroll), config_.zoom_speed);
        }
    }
} // namespace vulkan_engine::rendering