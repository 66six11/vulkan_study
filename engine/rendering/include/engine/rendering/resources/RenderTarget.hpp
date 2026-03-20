#pragma once

#include "engine/rhi/vulkan/device/Device.hpp"
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
     * @brief 娓叉煋鐩爣
     * 灏佽棰滆壊闄勪欢鍜屾繁搴﹂檮浠讹紝鐢ㄤ簬绂诲睆娓叉煋
     *
     * 鑱岃矗锛?
     * - 绠＄悊棰滆壊/娣卞害 Image 鍜?ImageView锛堜娇鐢?VMA锛?
     * - 绠＄悊鑷繁鐨?Framebuffer锛圧AII锛?
     * - 鎻愪緵灏哄淇℃伅
     * 
     * 娉ㄦ剰锛氫笉鐩存帴鏆撮湶 Vulkan 鍘熷鍙ユ焺锛岄€氳繃 VmaImage 璁块棶搴曞眰璧勬簮
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
                bool                  create_color = true; // 鏄惁鍒涘缓棰滆壊闄勪欢
                bool                  create_depth = true; // 鏄惁鍒涘缓娣卞害闄勪欢
            };

            RenderTarget();
            ~RenderTarget();

            // 绂佹鎷疯礉
            RenderTarget(const RenderTarget&)            = delete;
            RenderTarget& operator=(const RenderTarget&) = delete;

            // 鍏佽绉诲姩
            RenderTarget(RenderTarget&& other) noexcept;
            RenderTarget& operator=(RenderTarget&& other) noexcept;

            // 鍒濆鍖?
            void initialize(std::shared_ptr<vulkan::memory::VmaAllocator> allocator, const CreateInfo& info);

            // 娓呯悊璧勬簮
            void cleanup();

            // 閲嶆柊鍒涘缓锛堝昂瀵稿彉鍖栨椂锛?
            void resize(uint32_t width, uint32_t height);

            // Getters - 杩斿洖鏍煎紡/灏哄淇℃伅
            VkFormat              color_format() const { return color_format_; }
            VkFormat              depth_format() const { return depth_format_; }
            VkExtent2D            extent() const { return {width_, height_}; }
            uint32_t              width() const { return width_; }
            uint32_t              height() const { return height_; }
            VkSampleCountFlagBits samples() const { return samples_; }

            // 妫€鏌ユ槸鍚︽湁棰滆壊/娣卞害闄勪欢
            bool has_color() const { return color_image_ != nullptr; }
            bool has_depth() const { return depth_image_ != nullptr; }

            // 鑾峰彇鍥惧儚璧勬簮锛圧AII 灏佽锛屼笉鏆撮湶鍘熷鍙ユ焺锛?
            std::shared_ptr<vulkan::memory::VmaImage> color_image() const { return color_image_; }
            std::shared_ptr<vulkan::memory::VmaImage> depth_image() const { return depth_image_; }

            // 鑾峰彇 ImageView锛堢敤浜庢覆鏌擄級
            VkImageView color_image_view() const;
            VkImageView depth_image_view() const;

            // 鑾峰彇鍒嗛厤鍣?
            std::shared_ptr<vulkan::memory::VmaAllocator> allocator() const { return allocator_; }

            // Framebuffer 绠＄悊
            void          create_framebuffer(VkRenderPass render_pass);
            void          destroy_framebuffer();
            bool          has_framebuffer() const;
            VkFramebuffer framebuffer_handle() const;

        private:
            std::shared_ptr<vulkan::memory::VmaAllocator> allocator_;

            // 閰嶇疆
            uint32_t              width_        = 0;
            uint32_t              height_       = 0;
            VkFormat              color_format_ = VK_FORMAT_UNDEFINED;
            VkFormat              depth_format_ = VK_FORMAT_UNDEFINED;
            VkSampleCountFlagBits samples_      = VK_SAMPLE_COUNT_1_BIT;
            bool                  create_color_ = true;
            bool                  create_depth_ = true;

            // 棰滆壊闄勪欢锛堜娇鐢?VMA 绠＄悊锛?
            std::shared_ptr<vulkan::memory::VmaImage> color_image_;
            VkImageView                               color_image_view_ = VK_NULL_HANDLE;

            // 娣卞害闄勪欢锛堜娇鐢?VMA 绠＄悊锛?
            std::shared_ptr<vulkan::memory::VmaImage> depth_image_;
            VkImageView                               depth_image_view_ = VK_NULL_HANDLE;

            // Framebuffer锛圧AII 绠＄悊锛?
            std::unique_ptr<vulkan::Framebuffer> framebuffer_;

            void create_images();
            void create_color_image();
            void create_depth_image();
            void transition_image_layout();
    };
} // namespace vulkan_engine::rendering