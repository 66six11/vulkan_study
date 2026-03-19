/**
 * @file IImage.hpp
 * @brief Image 资源接口
 */

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief Image 子资源范围描述
     */
    struct ImageSubresourceRange
    {
        uint32_t aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t baseMipLevel   = 0;
        uint32_t levelCount     = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount     = 1;
    };

    /**
     * @brief Image 接口
     * 
     * 抽象的 GPU Image 资源，支持不同的实现
     */
    class IImage
    {
        public:
            virtual ~IImage() = default;

            // 禁止拷贝
            IImage(const IImage&)            = delete;
            IImage& operator=(const IImage&) = delete;

            // 句柄访问
            [[nodiscard]] virtual VkImage handle() const noexcept = 0;
            [[nodiscard]] virtual bool    isValid() const noexcept = 0;

            // 尺寸访问
            [[nodiscard]] virtual uint32_t width() const noexcept = 0;
            [[nodiscard]] virtual uint32_t height() const noexcept = 0;
            [[nodiscard]] virtual uint32_t depth() const noexcept = 0;
            [[nodiscard]] virtual uint32_t mipLevels() const noexcept = 0;
            [[nodiscard]] virtual uint32_t arrayLayers() const noexcept = 0;

            // 格式和属性
            [[nodiscard]] virtual int      format() const noexcept = 0;        // VkFormat
            [[nodiscard]] virtual uint32_t samples() const noexcept = 0;       // VkSampleCountFlagBits
            [[nodiscard]] virtual int      currentLayout() const noexcept = 0; // VkImageLayout

            // 布局转换
            virtual void setLayout(int newLayout) = 0; // VkImageLayout，仅设置跟踪状态
            virtual void transitionLayout(VkCommandBuffer cmd, int newLayout, const ImageSubresourceRange& range = {}) = 0;

            // Image View 管理
            [[nodiscard]] virtual VkImageView createView(
                int viewType,
                // VkImageViewType
                int format,
                // VkFormat，VK_FORMAT_UNDEFINED 表示使用 Image 格式
                const ImageSubresourceRange& range
            ) = 0;

            virtual void destroyView(VkImageView view) = 0;
            virtual void destroyAllViews() = 0;

            // 数据上传/下载（需要 Command Buffer）
            virtual void uploadData(const void* data, uint64_t size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0) = 0;
            virtual void downloadData(void* data, uint64_t size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0) = 0;

            // Mipmap 生成
            virtual void generateMipmaps(VkCommandBuffer cmd) = 0;

            // 获取分配信息
            [[nodiscard]] virtual const void* allocationInfo() const = 0;
    };

    using IImagePtr     = std::shared_ptr<IImage>;
    using IImageWeakPtr = std::weak_ptr<IImage>;

    /**
     * @brief 2D Texture 包装类
     */
    class ITexture2D
    {
        public:
            explicit ITexture2D(IImagePtr image)
                : image_(std::move(image))
            {
            }

            [[nodiscard]] IImage*   get() const noexcept { return image_.get(); }
            [[nodiscard]] IImagePtr shared() const noexcept { return image_; }
            [[nodiscard]] VkImage   handle() const noexcept { return image_ ? image_->handle() : VK_NULL_HANDLE; }
            [[nodiscard]] bool      isValid() const noexcept { return image_ && image_->isValid(); }

            [[nodiscard]] uint32_t width() const noexcept { return image_ ? image_->width() : 0; }
            [[nodiscard]] uint32_t height() const noexcept { return image_ ? image_->height() : 0; }
            [[nodiscard]] uint32_t mipLevels() const noexcept { return image_ ? image_->mipLevels() : 0; }
            [[nodiscard]] int      format() const noexcept { return image_ ? image_->format() : VK_FORMAT_UNDEFINED; }

            [[nodiscard]] VkImageView defaultView() const
            {
                if (!defaultView_ && image_)
                {
                    ImageSubresourceRange range;
                    range.aspectMask                            = VK_IMAGE_ASPECT_COLOR_BIT;
                    range.levelCount                            = image_->mipLevels();
                    const_cast<ITexture2D*>(this)->defaultView_ = image_->createView(
                                                                                     VK_IMAGE_VIEW_TYPE_2D,
                                                                                     image_->format(),
                                                                                     range
                                                                                    );
                }
                return defaultView_;
            }

        private:
            IImagePtr   image_;
            VkImageView defaultView_ = VK_NULL_HANDLE;
    };
} // namespace vulkan_engine::vulkan::memory