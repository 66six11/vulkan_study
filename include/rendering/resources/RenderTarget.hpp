#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace vulkan_engine::vulkan
{
    // Forward declaration to avoid direct dependency on Vulkan backend details
    class Framebuffer;
}

namespace vulkan_engine::vulkan::memory
{
    // Forward declarations
    class VmaAllocator;
    class VmaImage;
}

namespace vulkan_engine::rendering
{
    /**
     * @brief 渲染目标
     * 封装颜色附件和深度附件，用于离屏渲染
     *
     * 职责：
     * - 管理颜色/深度 Image 和 ImageView（使用 VMA）
     * - 管理自己的 Framebuffer（RAII）
     * - 提供尺寸信息
     * 
     * 注意：不直接暴露 Vulkan 原始句柄，通过 VmaImage 访问底层资源
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
            void initialize(std::shared_ptr<vulkan::memory::VmaAllocator> allocator, const CreateInfo& info);

            // 清理资源
            void cleanup();

            // 重新创建（尺寸变化时）
            void resize(uint32_t width, uint32_t height);

            // Getters - 返回格式/尺寸信息
            VkFormat              color_format() const { return color_format_; }
            VkFormat              depth_format() const { return depth_format_; }
            VkExtent2D            extent() const { return {width_, height_}; }
            uint32_t              width() const { return width_; }
            uint32_t              height() const { return height_; }
            VkSampleCountFlagBits samples() const { return samples_; }

            // 检查是否有颜色/深度附件
            bool has_color() const { return color_image_ != nullptr; }
            bool has_depth() const { return depth_image_ != nullptr; }

            // 获取图像资源（RAII 封装，不暴露原始句柄）
            std::shared_ptr<vulkan::memory::VmaImage> color_image() const { return color_image_; }
            std::shared_ptr<vulkan::memory::VmaImage> depth_image() const { return depth_image_; }

            // 获取 ImageView（用于渲染）
            VkImageView color_image_view() const;
            VkImageView depth_image_view() const;

            // 获取分配器
            std::shared_ptr<vulkan::memory::VmaAllocator> allocator() const { return allocator_; }

            // Framebuffer 管理
            void          create_framebuffer(VkRenderPass render_pass);
            void          destroy_framebuffer();
            bool          has_framebuffer() const;
            VkFramebuffer framebuffer_handle() const;

        private:
            std::shared_ptr<vulkan::memory::VmaAllocator> allocator_;

            // 配置
            uint32_t              width_        = 0;
            uint32_t              height_       = 0;
            VkFormat              color_format_ = VK_FORMAT_UNDEFINED;
            VkFormat              depth_format_ = VK_FORMAT_UNDEFINED;
            VkSampleCountFlagBits samples_      = VK_SAMPLE_COUNT_1_BIT;
            bool                  create_color_ = true;
            bool                  create_depth_ = true;

            // 颜色附件（使用 VMA 管理）
            std::shared_ptr<vulkan::memory::VmaImage> color_image_;
            VkImageView                               color_image_view_ = VK_NULL_HANDLE;

            // 深度附件（使用 VMA 管理）
            std::shared_ptr<vulkan::memory::VmaImage> depth_image_;
            VkImageView                               depth_image_view_ = VK_NULL_HANDLE;

            // Framebuffer（RAII 管理）
            std::unique_ptr<vulkan::Framebuffer> framebuffer_;

            void create_images();
            void create_color_image();
            void create_depth_image();
            void transition_image_layout();
    };
} // namespace vulkan_engine::rendering