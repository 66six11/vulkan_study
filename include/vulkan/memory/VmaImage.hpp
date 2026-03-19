#pragma once

#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/memory/Allocation.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <vector>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // Image 子资源范围
    struct ImageSubresourceRange
    {
        VkImageAspectFlags aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t           baseMipLevel   = 0;
        uint32_t           levelCount     = 1;
        uint32_t           baseArrayLayer = 0;
        uint32_t           layerCount     = 1;

        static ImageSubresourceRange colorAll(uint32_t mipLevels = 1, uint32_t arrayLayers = 1)
        {
            return {VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, arrayLayers};
        }

        static ImageSubresourceRange depthAll(uint32_t mipLevels = 1, uint32_t arrayLayers = 1)
        {
            return {VK_IMAGE_ASPECT_DEPTH_BIT, 0, mipLevels, 0, arrayLayers};
        }

        static ImageSubresourceRange stencilAll(uint32_t mipLevels = 1, uint32_t arrayLayers = 1)
        {
            return {VK_IMAGE_ASPECT_STENCIL_BIT, 0, mipLevels, 0, arrayLayers};
        }

        static ImageSubresourceRange depthStencilAll(uint32_t mipLevels = 1, uint32_t arrayLayers = 1)
        {
            return {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, mipLevels, 0, arrayLayers};
        }
    };

    // VMA Image 类
    class VmaImage
    {
        public:
            VmaImage(
                std::shared_ptr<VmaAllocator>  allocator,
                const VkImageCreateInfo&       imageInfo,
                const VmaAllocationCreateInfo& allocInfo);
            ~VmaImage();

            // Non-copyable
            VmaImage(const VmaImage&)            = delete;
            VmaImage& operator=(const VmaImage&) = delete;

            // Movable
            VmaImage(VmaImage&& other) noexcept;
            VmaImage& operator=(VmaImage&& other) noexcept;

            // Image View 管理
            VkImageView createView(VkImageViewType viewType, VkFormat format, const ImageSubresourceRange& range);
            VkImageView createView(VkImageViewType viewType, const ImageSubresourceRange& range) { return createView(viewType, format_, range); }
            void        destroyView(VkImageView view);
            void        destroyAllViews();

            // 获取默认 view（创建的第一个 view）
            VkImageView defaultView() const { return views_.empty() ? VK_NULL_HANDLE : views_[0]; }

            // 布局转换
            void          setLayout(VkImageLayout newLayout) { currentLayout_ = newLayout; }
            void          transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, const ImageSubresourceRange& range = {});
            VkImageLayout currentLayout() const { return currentLayout_; }

            // 生成 Mipmaps
            void generateMipmaps(VkCommandBuffer cmd);

            // 数据上传/下载
            void uploadData(const void* data, VkDeviceSize size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
            void downloadData(void* data, VkDeviceSize size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

            // 访问器
            VkImage               handle() const noexcept { return image_; }
            VkFormat              format() const noexcept { return format_; }
            VkExtent3D            extent() const noexcept { return extent_; }
            uint32_t              width() const noexcept { return extent_.width; }
            uint32_t              height() const noexcept { return extent_.height; }
            uint32_t              depth() const noexcept { return extent_.depth; }
            uint32_t              mipLevels() const noexcept { return mipLevels_; }
            uint32_t              arrayLayers() const noexcept { return arrayLayers_; }
            VkSampleCountFlagBits samples() const noexcept { return samples_; }
            VkImageUsageFlags     usage() const noexcept { return usage_; }
            VkImageType           imageType() const noexcept { return imageType_; }
            bool                  isValid() const noexcept { return image_ != VK_NULL_HANDLE && allocation_.isValid(); }

            AllocationInfo    allocationInfo() const { return allocationInfo_; }
            const Allocation& allocation() const { return allocation_; }

            // 获取子资源大小
            VkDeviceSize getSubresourceSize(uint32_t mipLevel = 0) const;

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VkImage                       image_ = VK_NULL_HANDLE;
            Allocation                    allocation_;
            std::vector<VkImageView>      views_;

            // Image 属性
            VkFormat              format_        = VK_FORMAT_UNDEFINED;
            VkExtent3D            extent_        = {0, 0, 0};
            uint32_t              mipLevels_     = 1;
            uint32_t              arrayLayers_   = 1;
            VkSampleCountFlagBits samples_       = VK_SAMPLE_COUNT_1_BIT;
            VkImageUsageFlags     usage_         = 0;
            VkImageType           imageType_     = VK_IMAGE_TYPE_2D;
            VkImageLayout         currentLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
            AllocationInfo        allocationInfo_{};

            void cleanup() noexcept;

            // 辅助函数
            static VkAccessFlags        getAccessMask(VkImageLayout layout);
            static VkPipelineStageFlags getStageMask(VkImageLayout layout);
    };

    using VmaImagePtr = std::shared_ptr<VmaImage>;

    // Image 构建器
    class VmaImageBuilder
    {
        public:
            explicit VmaImageBuilder(std::shared_ptr<VmaAllocator> allocator);

            // 基本配置
            VmaImageBuilder& type(VkImageType type);
            VmaImageBuilder& extent(uint32_t width, uint32_t height, uint32_t depth = 1);
            VmaImageBuilder& width(uint32_t width);
            VmaImageBuilder& height(uint32_t height);
            VmaImageBuilder& depth(uint32_t depth);
            VmaImageBuilder& format(VkFormat format);
            VmaImageBuilder& usage(VkImageUsageFlags usage);
            VmaImageBuilder& mipLevels(uint32_t levels);
            VmaImageBuilder& arrayLayers(uint32_t layers);
            VmaImageBuilder& samples(VkSampleCountFlagBits samples);
            VmaImageBuilder& tiling(VkImageTiling tiling);
            VmaImageBuilder& initialLayout(VkImageLayout layout);

            // 内存配置
            VmaImageBuilder& deviceLocal();
            VmaImageBuilder& hostVisible();
            VmaImageBuilder& dedicatedMemory();

            // 高级选项
            VmaImageBuilder& pool(VmaPool pool);
            VmaImageBuilder& priority(float priority);
            VmaImageBuilder& allocationFlags(VmaAllocationCreateFlags flags);

            // 构建
            std::unique_ptr<VmaImage> build();
            VmaImagePtr               buildShared();

            // 预设配置
            static std::unique_ptr<VmaImage> createColorAttachment(
                std::shared_ptr<VmaAllocator> allocator,
                uint32_t                      width,
                uint32_t                      height,
                VkFormat                      format,
                uint32_t                      mipLevels = 1,
                VkSampleCountFlagBits         samples   = VK_SAMPLE_COUNT_1_BIT);

            static std::unique_ptr<VmaImage> createDepthAttachment(
                std::shared_ptr<VmaAllocator> allocator,
                uint32_t                      width,
                uint32_t                      height,
                VkFormat                      format,
                VkSampleCountFlagBits         samples = VK_SAMPLE_COUNT_1_BIT);

            static std::unique_ptr<VmaImage> createTexture(
                std::shared_ptr<VmaAllocator> allocator,
                uint32_t                      width,
                uint32_t                      height,
                VkFormat                      format,
                uint32_t                      mipLevels   = 1,
                uint32_t                      arrayLayers = 1);

            static std::unique_ptr<VmaImage> createCubemap(
                std::shared_ptr<VmaAllocator> allocator,
                uint32_t                      size,
                VkFormat                      format,
                uint32_t                      mipLevels = 1);

            static std::unique_ptr<VmaImage> createStorageImage(
                std::shared_ptr<VmaAllocator> allocator,
                uint32_t                      width,
                uint32_t                      height,
                VkFormat                      format);

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VkImageCreateInfo             imageInfo_{};
            VmaAllocationCreateInfo       allocInfo_{};
            float                         priority_ = 0.5f;

            void resetInfo();
    };

    // Image View 描述符
    struct ImageViewDescriptor
    {
        VkImageViewType       viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkFormat              format   = VK_FORMAT_UNDEFINED; // 使用 image 的 format
        ImageSubresourceRange range;
        VkComponentMapping    components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
    };
} // namespace vulkan_engine::vulkan::memory