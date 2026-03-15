#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace vulkan_engine::rendering
{
    /**
     * @brief 渲染目标
     * 封装颜色附件和深度附件，用于离屏渲染
     *
     * 职责：
     * - 管理颜色/深度 Image 和 ImageView
     * - 提供尺寸信息
     * - 不参与 RenderPass/Framebuffer 管理
     */
    class RenderTarget
    {
        public:
            struct CreateInfo
            {
                uint32_t              width        = 1280;
                uint32_t              height       = 720;
                VkFormat              color_format = VK_FORMAT_B8G8R8A8_UNORM;
                VkFormat              depth_format = VK_FORMAT_D32_SFLOAT;
                VkSampleCountFlagBits samples      = VK_SAMPLE_COUNT_1_BIT;
                bool                  create_color = true; // 是否创建颜色附件
                bool                  create_depth = true; // 是否创建深度附件
            };

            RenderTarget();
            ~RenderTarget();

            // 禁止拷贝
            RenderTarget(const RenderTarget&)            = delete;
            RenderTarget& operator=(const RenderTarget&) = delete;

            // 允许移动
            RenderTarget(RenderTarget&& other) noexcept;
            RenderTarget& operator=(RenderTarget&& other) noexcept;

            // 初始化
            void initialize(std::shared_ptr<vulkan::DeviceManager> device, const CreateInfo& info);

            // 清理资源
            void cleanup();

            // 重新创建（尺寸变化时）
            void resize(uint32_t width, uint32_t height);

            // Getters
            VkImageView           color_image_view() const { return color_image_view_; }
            VkImageView           depth_image_view() const { return depth_image_view_; }
            VkImage               color_image() const { return color_image_; }
            VkImage               depth_image() const { return depth_image_; }
            VkFormat              color_format() const { return color_format_; }
            VkFormat              depth_format() const { return depth_format_; }
            VkExtent2D            extent() const { return {width_, height_}; }
            uint32_t              width() const { return width_; }
            uint32_t              height() const { return height_; }
            VkSampleCountFlagBits samples() const { return samples_; }

            bool has_color() const { return color_image_ != VK_NULL_HANDLE; }
            bool has_depth() const { return depth_image_ != VK_NULL_HANDLE; }

            // 获取设备
            std::shared_ptr<vulkan::DeviceManager> device() const { return device_; }

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;

            // 配置
            uint32_t              width_        = 0;
            uint32_t              height_       = 0;
            VkFormat              color_format_ = VK_FORMAT_UNDEFINED;
            VkFormat              depth_format_ = VK_FORMAT_UNDEFINED;
            VkSampleCountFlagBits samples_      = VK_SAMPLE_COUNT_1_BIT;
            bool                  create_color_ = true;
            bool                  create_depth_ = true;

            // 颜色附件
            VkImage        color_image_      = VK_NULL_HANDLE;
            VkDeviceMemory color_memory_     = VK_NULL_HANDLE;
            VkImageView    color_image_view_ = VK_NULL_HANDLE;

            // 深度附件
            VkImage        depth_image_      = VK_NULL_HANDLE;
            VkDeviceMemory depth_memory_     = VK_NULL_HANDLE;
            VkImageView    depth_image_view_ = VK_NULL_HANDLE;

            void create_images();
            void create_color_image();
            void create_depth_image();
            void transition_image_layout();
    };
} // namespace vulkan_engine::rendering