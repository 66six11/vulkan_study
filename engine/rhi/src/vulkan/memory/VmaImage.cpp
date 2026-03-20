#include "vulkan/memory/VmaImage.hpp"
#include "vulkan/memory/VmaBuffer.hpp"
#include "core/utils/Logger.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace vulkan_engine::vulkan::memory
{
    VmaImage::VmaImage(
        std::shared_ptr<VmaAllocator>  allocator,
        const VkImageCreateInfo&       imageInfo,
        const VmaAllocationCreateInfo& allocInfo)
        : allocator_(std::move(allocator))
        , format_(imageInfo.format)
        , extent_(imageInfo.extent)
        , mipLevels_(imageInfo.mipLevels)
        , arrayLayers_(imageInfo.arrayLayers)
        , samples_(imageInfo.samples)
        , usage_(imageInfo.usage)
        , imageType_(imageInfo.imageType)
        , currentLayout_(imageInfo.initialLayout)
    {
        if (!allocator_)
        {
            throw std::runtime_error("VmaImage: allocator is null");
        }

        VmaAllocation     allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo;

        VkResult result = vmaCreateImage(allocator_->handle(), &imageInfo, &allocInfo, &image_, &allocation, &allocationInfo);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create VMA image", __FILE__, __LINE__);
        }

        allocation_ = Allocation(allocator_, allocation);

        // 存储分配信息
        allocationInfo_.size            = allocationInfo.size;
        allocationInfo_.memoryTypeIndex = allocationInfo.memoryType;
        allocationInfo_.mappedData      = allocationInfo.pMappedData;

        std::ostringstream oss;
        oss << "VmaImage created: " << extent_.width << "x" << extent_.height << "x" << extent_.depth
                << ", format=" << format_ << ", mips=" << mipLevels_
                << ", memoryType=" << allocationInfo_.memoryTypeIndex;
        LOG_DEBUG(oss.str());
    }

    VmaImage::~VmaImage()
    {
        cleanup();
    }

    VmaImage::VmaImage(VmaImage&& other) noexcept
        : allocator_(std::move(other.allocator_))
        , image_(other.image_)
        , allocation_(std::move(other.allocation_))
        , views_(std::move(other.views_))
        , format_(other.format_)
        , extent_(other.extent_)
        , mipLevels_(other.mipLevels_)
        , arrayLayers_(other.arrayLayers_)
        , samples_(other.samples_)
        , usage_(other.usage_)
        , imageType_(other.imageType_)
        , currentLayout_(other.currentLayout_)
        , allocationInfo_(other.allocationInfo_)
    {
        other.image_ = VK_NULL_HANDLE;
        other.views_.clear();
    }

    VmaImage& VmaImage::operator=(VmaImage&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            allocator_      = std::move(other.allocator_);
            image_          = other.image_;
            allocation_     = std::move(other.allocation_);
            views_          = std::move(other.views_);
            format_         = other.format_;
            extent_         = other.extent_;
            mipLevels_      = other.mipLevels_;
            arrayLayers_    = other.arrayLayers_;
            samples_        = other.samples_;
            usage_          = other.usage_;
            imageType_      = other.imageType_;
            currentLayout_  = other.currentLayout_;
            allocationInfo_ = other.allocationInfo_;
            other.image_    = VK_NULL_HANDLE;
            other.views_.clear();
        }
        return *this;
    }

    void VmaImage::cleanup() noexcept
    {
        // 销毁所有 view
        for (VkImageView view : views_)
        {
            if (view != VK_NULL_HANDLE && allocator_)
            {
                vkDestroyImageView(allocator_->device()->device().handle(), view, nullptr);
            }
        }
        views_.clear();

        // 销毁 image（allocation 会自动释放）
        if (image_ != VK_NULL_HANDLE)
        {
            if (auto allocator = allocator_)
            {
                vmaDestroyImage(allocator->handle(), image_, VK_NULL_HANDLE);
            }
            image_ = VK_NULL_HANDLE;
        }
    }

    VkImageView VmaImage::createView(VkImageViewType viewType, VkFormat format, const ImageSubresourceRange& range)
    {
        VkImageViewCreateInfo viewInfo           = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = image_;
        viewInfo.viewType                        = viewType;
        viewInfo.format                          = format == VK_FORMAT_UNDEFINED ? format_ : format;
        viewInfo.subresourceRange.aspectMask     = range.aspectMask;
        viewInfo.subresourceRange.baseMipLevel   = range.baseMipLevel;
        viewInfo.subresourceRange.levelCount     = range.levelCount;
        viewInfo.subresourceRange.baseArrayLayer = range.baseArrayLayer;
        viewInfo.subresourceRange.layerCount     = range.layerCount;

        VkImageView view   = VK_NULL_HANDLE;
        VkResult    result = vkCreateImageView(allocator_->device()->device().handle(), &viewInfo, nullptr, &view);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create image view", __FILE__, __LINE__);
        }

        views_.push_back(view);
        return view;
    }

    void VmaImage::destroyView(VkImageView view)
    {
        auto it = std::find(views_.begin(), views_.end(), view);
        if (it != views_.end())
        {
            vkDestroyImageView(allocator_->device()->device().handle(), view, nullptr);
            views_.erase(it);
        }
    }

    void VmaImage::destroyAllViews()
    {
        for (VkImageView view : views_)
        {
            if (view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(allocator_->device()->device().handle(), view, nullptr);
            }
        }
        views_.clear();
    }

    void VmaImage::transitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, const ImageSubresourceRange& range)
    {
        if (currentLayout_ == newLayout)
        {
            return;
        }

        ImageSubresourceRange r = range;
        if (r.levelCount == 0) r.levelCount = mipLevels_;
        if (r.layerCount == 0) r.layerCount = arrayLayers_;
        if (r.aspectMask == 0)
        {
            r.aspectMask = (format_ == VK_FORMAT_D32_SFLOAT || format_ == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                            format_ == VK_FORMAT_D24_UNORM_S8_UINT)
                               ? VK_IMAGE_ASPECT_DEPTH_BIT
                               : VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkAccessFlags        srcAccess = getAccessMask(currentLayout_);
        VkAccessFlags        dstAccess = getAccessMask(newLayout);
        VkPipelineStageFlags srcStage  = getStageMask(currentLayout_);
        VkPipelineStageFlags dstStage  = getStageMask(newLayout);

        VkImageMemoryBarrier barrier = {};
        barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout            = currentLayout_;
        barrier.newLayout            = newLayout;
        barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                = image_;
        barrier.subresourceRange     = VkImageSubresourceRange{
            r.aspectMask,
            r.baseMipLevel,
            r.levelCount,
            r.baseArrayLayer,
            r.layerCount
        };
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        currentLayout_ = newLayout;
    }

    void VmaImage::generateMipmaps(VkCommandBuffer cmd)
    {
        if (mipLevels_ <= 1)
        {
            return;
        }

        // 确保 image 有 TRANSFER_SRC 和 TRANSFER_DST 用法
        if ((usage_ & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0 || (usage_ & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
        {
            LOG_WARN("VmaImage::generateMipmaps: image must have TRANSFER_SRC and TRANSFER_DST usage");
            return;
        }

        ImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = arrayLayers_;

        int32_t mipWidth  = static_cast<int32_t>(extent_.width);
        int32_t mipHeight = static_cast<int32_t>(extent_.height);

        for (uint32_t i = 1; i < mipLevels_; ++i)
        {
            // 转换前一 mip 到 SRC_OPTIMAL
            range.baseMipLevel = i - 1;
            transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range);

            // 转换当前 mip 到 DST_OPTIMAL
            range.baseMipLevel = i;
            transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

            VkImageBlit blit                   = {};
            blit.srcOffsets[0]                 = {0, 0, 0};
            blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = arrayLayers_;

            blit.dstOffsets[0]                 = {0, 0, 0};
            blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = arrayLayers_;

            vkCmdBlitImage(cmd,
                           image_,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image_,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            mipWidth  = mipWidth > 1 ? mipWidth / 2 : 1;
            mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
        }
    }

    void VmaImage::uploadData(const void* data, VkDeviceSize size, uint32_t mipLevel, uint32_t arrayLayer)
    {
        (void)mipLevel;
        (void)arrayLayer;

        // 创建 staging buffer
        auto stagingBuffer = VmaBufferBuilder::createStagingBuffer(allocator_, size);
        stagingBuffer->write(data, size);

        // 注意：实际的上传需要 command buffer，这里只是一个占位符
        // 完整的实现应该在 CommandBuffer 类中
        std::ostringstream oss;
        oss << "VmaImage::uploadData: staging buffer created, size=" << size;
        LOG_DEBUG(oss.str());
    }

    void VmaImage::downloadData(void* data, VkDeviceSize size, uint32_t mipLevel, uint32_t arrayLayer)
    {
        (void)data;
        (void)size;
        (void)mipLevel;
        (void)arrayLayer;

        // 类似 upload，需要 staging buffer 和 command buffer
        LOG_DEBUG("VmaImage::downloadData: not implemented yet");
    }

    VkDeviceSize VmaImage::getSubresourceSize(uint32_t mipLevel) const
    {
        // 使用括号避免 Windows 宏冲突
        uint32_t width  = (std::max)(1u, extent_.width >> mipLevel);
        uint32_t height = (std::max)(1u, extent_.height >> mipLevel);
        uint32_t depth  = (std::max)(1u, extent_.depth >> mipLevel);

        // 粗略估计（实际需要根据格式计算）
        uint32_t bytesPerPixel = 4; // 假设 4 bytes per pixel
        return static_cast<VkDeviceSize>(width * height * depth * bytesPerPixel);
    }

    VkAccessFlags VmaImage::getAccessMask(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                return 0;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_ACCESS_TRANSFER_READ_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                return VK_ACCESS_SHADER_READ_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return VK_ACCESS_MEMORY_READ_BIT;
            default:
                return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        }
    }

    VkPipelineStageFlags VmaImage::getStageMask(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            default:
                return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
    }

    // VmaImageBuilder 实现
    VmaImageBuilder::VmaImageBuilder(std::shared_ptr<VmaAllocator> allocator)
        : allocator_(std::move(allocator))
    {
        resetInfo();
    }

    void VmaImageBuilder::resetInfo()
    {
        imageInfo_               = {};
        imageInfo_.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo_.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo_.extent        = {1, 1, 1};
        imageInfo_.mipLevels     = 1;
        imageInfo_.arrayLayers   = 1;
        imageInfo_.format        = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo_.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo_.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo_.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo_.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        allocInfo_       = {};
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    }

    VmaImageBuilder& VmaImageBuilder::type(VkImageType type)
    {
        imageInfo_.imageType = type;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::extent(uint32_t width, uint32_t height, uint32_t depth)
    {
        imageInfo_.extent = {width, height, depth};
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::width(uint32_t width)
    {
        imageInfo_.extent.width = width;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::height(uint32_t height)
    {
        imageInfo_.extent.height = height;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::depth(uint32_t depth)
    {
        imageInfo_.extent.depth = depth;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::format(VkFormat format)
    {
        imageInfo_.format = format;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::usage(VkImageUsageFlags usage)
    {
        imageInfo_.usage = usage;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::mipLevels(uint32_t levels)
    {
        imageInfo_.mipLevels = levels;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::arrayLayers(uint32_t layers)
    {
        imageInfo_.arrayLayers = layers;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::samples(VkSampleCountFlagBits samples)
    {
        imageInfo_.samples = samples;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::tiling(VkImageTiling tiling)
    {
        imageInfo_.tiling = tiling;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::initialLayout(VkImageLayout layout)
    {
        imageInfo_.initialLayout = layout;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::deviceLocal()
    {
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo_.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::hostVisible()
    {
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo_.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::dedicatedMemory()
    {
        allocInfo_.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::pool(VmaPool pool)
    {
        allocInfo_.pool = pool;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::priority(float priority)
    {
        allocInfo_.priority = priority;
        return *this;
    }

    VmaImageBuilder& VmaImageBuilder::allocationFlags(VmaAllocationCreateFlags flags)
    {
        allocInfo_.flags = flags;
        return *this;
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::build()
    {
        return std::make_unique < VmaImage > (allocator_, imageInfo_, allocInfo_);
    }

    VmaImagePtr VmaImageBuilder::buildShared()
    {
        return std::make_shared < VmaImage > (allocator_, imageInfo_, allocInfo_);
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::createColorAttachment(
        std::shared_ptr<VmaAllocator> allocator,
        uint32_t                      width,
        uint32_t                      height,
        VkFormat                      format,
        uint32_t                      mipLevels,
        VkSampleCountFlagBits         samples)
    {
        return VmaImageBuilder(allocator)
              .width(width)
              .height(height)
              .format(format)
              .mipLevels(mipLevels)
              .samples(samples)
              .usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
              .deviceLocal()
              .build();
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::createDepthAttachment(
        std::shared_ptr<VmaAllocator> allocator,
        uint32_t                      width,
        uint32_t                      height,
        VkFormat                      format,
        VkSampleCountFlagBits         samples)
    {
        return VmaImageBuilder(allocator)
              .width(width)
              .height(height)
              .format(format)
              .samples(samples)
              .usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
              .deviceLocal()
              .build();
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::createTexture(
        std::shared_ptr<VmaAllocator> allocator,
        uint32_t                      width,
        uint32_t                      height,
        VkFormat                      format,
        uint32_t                      mipLevels,
        uint32_t                      arrayLayers)
    {
        return VmaImageBuilder(allocator)
              .width(width)
              .height(height)
              .format(format)
              .mipLevels(mipLevels)
              .arrayLayers(arrayLayers)
              .usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
              .deviceLocal()
              .build();
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::createCubemap(
        std::shared_ptr<VmaAllocator> allocator,
        uint32_t                      size,
        VkFormat                      format,
        uint32_t                      mipLevels)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageInfo.extent            = {size, size, 1};
        imageInfo.mipLevels         = mipLevels;
        imageInfo.arrayLayers       = 6;
        imageInfo.format            = format;
        imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        return std::make_unique < VmaImage > (allocator, imageInfo, allocInfo);
    }

    std::unique_ptr<VmaImage> VmaImageBuilder::createStorageImage(
        std::shared_ptr<VmaAllocator> allocator,
        uint32_t                      width,
        uint32_t                      height,
        VkFormat                      format)
    {
        return VmaImageBuilder(allocator)
              .width(width)
              .height(height)
              .format(format)
              .usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
              .deviceLocal()
              .build();
    }
} // namespace vulkan_engine::vulkan::memory