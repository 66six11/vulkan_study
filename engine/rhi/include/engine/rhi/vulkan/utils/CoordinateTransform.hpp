#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vulkan_engine::vulkan
{
    /**
     * @brief Vulkan 鍧愭爣绯昏浆鎹㈠伐鍏?
     * 澶勭悊 OpenGL 涓?Vulkan 鍧愭爣绯诲樊寮?
     */
    class CoordinateTransform
    {
        public:
            /**
         * @brief 灏?OpenGL 鎶曞奖鐭╅樀杞崲涓?Vulkan 鎶曞奖鐭╅樀
         * Vulkan 鐨?Y 杞存寚鍚戝睆骞曚笅鏂癸紝涓?OpenGL 鐩稿弽
         * @param proj OpenGL 椋庢牸鐨勬姇褰辩煩闃?
         * @return Vulkan 椋庢牸鐨勬姇褰辩煩闃?
         */
            static glm::mat4 opengl_to_vulkan_projection(const glm::mat4& proj)
            {
                glm::mat4 result = proj;
                result[1][1] *= -1.0f; // 缈昏浆 Y 杞?
                return result;
            }

            /**
         * @brief 鍒涘缓 Vulkan 鍏煎鐨勬浜ゆ姇褰辩煩闃?
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
         * @brief 鍒涘缓 Vulkan 鍏煎鐨勯€忚鎶曞奖鐭╅樀
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
         * @brief 鑾峰彇 Y 杞寸炕杞煩闃碉紙鐢ㄤ簬鎵嬪姩澶勭悊锛?
         */
            static glm::mat4 get_y_flip_matrix()
            {
                glm::mat4 flip(1.0f);
                flip[1][1] = -1.0f;
                return flip;
            }
    };
} // namespace vulkan_engine::vulkan