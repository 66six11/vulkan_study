#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vulkan_engine::vulkan
{
    /**
     * @brief Vulkan 坐标系转换工具
     * 处理 OpenGL 与 Vulkan 坐标系差异
     */
    class CoordinateTransform
    {
        public:
            /**
         * @brief 将 OpenGL 投影矩阵转换为 Vulkan 投影矩阵
         * Vulkan 的 Y 轴指向屏幕下方，与 OpenGL 相反
         * @param proj OpenGL 风格的投影矩阵
         * @return Vulkan 风格的投影矩阵
         */
            static glm::mat4 opengl_to_vulkan_projection(const glm::mat4& proj)
            {
                glm::mat4 result = proj;
                result[1][1] *= -1.0f; // 翻转 Y 轴
                return result;
            }

            /**
         * @brief 创建 Vulkan 兼容的正交投影矩阵
         */
            static glm::mat4 ortho(
                float left,
                float right,
                float bottom,
                float top,
                float z_near,
                float z_far)
            {
                glm::mat4 proj = glm::ortho(left, right, bottom, top, z_near, z_far);
                return opengl_to_vulkan_projection(proj);
            }

            /**
         * @brief 创建 Vulkan 兼容的透视投影矩阵
         */
            static glm::mat4 perspective(
                float fov_y_degrees,
                float aspect_ratio,
                float z_near,
                float z_far)
            {
                glm::mat4 proj = glm::perspective(
                                                  glm::radians(fov_y_degrees),
                                                  aspect_ratio,
                                                  z_near,
                                                  z_far
                                                 );
                return opengl_to_vulkan_projection(proj);
            }

            /**
         * @brief 获取 Y 轴翻转矩阵（用于手动处理）
         */
            static glm::mat4 get_y_flip_matrix()
            {
                glm::mat4 flip(1.0f);
                flip[1][1] = -1.0f;
                return flip;
            }
    };
} // namespace vulkan_engine::vulkan