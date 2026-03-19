#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace vulkan_engine::core
{
    // Orbit camera for inspecting 3D objects
    // Mouse drag to rotate around target, scroll to zoom
    class OrbitCamera
    {
        public:
            OrbitCamera() = default;

            explicit OrbitCamera(const glm::vec3& target, float distance = 3.0f)
                : target_(target)
                , distance_(distance)
            {
            }

            // Get view matrix (for rendering)
            glm::mat4 get_view_matrix() const
            {
                glm::vec3 position = get_position();
                return glm::lookAt(position, target_, glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // Get projection matrix (API-agnostic, returns OpenGL-style projection)
            // Note: For Vulkan, use CoordinateTransform::opengl_to_vulkan_projection()
            glm::mat4 get_projection_matrix(float fov_degrees, float aspect_ratio, float near_plane, float far_plane) const
            {
                return glm::perspective(glm::radians(fov_degrees), aspect_ratio, near_plane, far_plane);
            }

            // Get combined view-projection matrix
            glm::mat4 get_view_projection_matrix(float fov_degrees, float aspect_ratio, float near_plane, float far_plane) const
            {
                return get_projection_matrix(fov_degrees, aspect_ratio, near_plane, far_plane) * get_view_matrix();
            }

            // Get camera position in world space
            glm::vec3 get_position() const
            {
                // Calculate position based on spherical coordinates
                float yaw_rad   = glm::radians(yaw_);
                float pitch_rad = glm::radians(pitch_);

                glm::vec3 offset;
                offset.x = distance_ * cos(pitch_rad) * sin(yaw_rad);
                offset.y = distance_ * sin(pitch_rad);
                offset.z = distance_ * cos(pitch_rad) * cos(yaw_rad);

                return target_ + offset;
            }

            // Mouse input handling
            void on_mouse_drag(float delta_x, float delta_y, float sensitivity = 0.5f)
            {
                yaw_ += delta_x * sensitivity;
                pitch_ += delta_y * sensitivity;

                // Clamp pitch to prevent flipping
                pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
            }

            void on_mouse_scroll(float scroll_delta, float zoom_speed = 0.1f)
            {
                distance_ -= scroll_delta * zoom_speed * distance_;
                distance_ = std::clamp(distance_, min_distance_, max_distance_);
            }

            // Setters
            void set_target(const glm::vec3& target) { target_ = target; }
            void set_distance(float distance) { distance_ = std::clamp(distance, min_distance_, max_distance_); }

            void set_rotation(float yaw, float pitch)
            {
                yaw_   = yaw;
                pitch_ = std::clamp(pitch, -89.0f, 89.0f);
            }

            // Getters
            const glm::vec3& get_target() const { return target_; }
            float            get_distance() const { return distance_; }
            float            get_yaw() const { return yaw_; }
            float            get_pitch() const { return pitch_; }

            void set_distance_limits(float min_dist, float max_dist)
            {
                min_distance_ = min_dist;
                max_distance_ = max_dist;
                distance_     = std::clamp(distance_, min_distance_, max_distance_);
            }

        private:
            glm::vec3 target_   = glm::vec3(0.0f, 0.0f, 0.0f); // Point to orbit around
            float     distance_ = 3.0f;                        // Distance from target
            float     yaw_      = 45.0f;                       // Horizontal rotation (degrees)
            float     pitch_    = -30.0f;                      // Vertical rotation (degrees), negative to look from above

            float min_distance_ = 0.5f;
            float max_distance_ = 20.0f;
    };
} // namespace vulkan_engine::core