#include "engine/rhi/vulkan/resources/DepthBuffer.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"
#include <algorithm>

namespace vulkan_engine::vulkan
{
    DepthBuffer::DepthBuffer(
        std::shared_ptr<DeviceManager> device,
        uint32_t                       width,
        uint32_t                       height)
        : device_(std::move(device))
        , width_(width)
        , height_(height)
        , format_(find_depth_format(device_))
    {
        create_image();
        create_view();
    }

    DepthBuffer::~DepthBuffer()
    {
        if (view_ != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device_->device(), view_, nullptr);
        }
        if (image_ != VK_NULL_HANDLE)
        {
            vkDestroyImage(device_->device(), image_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(device_->device(), memory_, nullptr);
        }
    }

    DepthBuffer::DepthBuffer(DepthBuffer&& other) noexcept
        : device_(std::move(other.device_))
        , image_(other.image_)
        , view_(other.view_)
        , memory_(other.memory_)
        , format_(other.format_)
        , width_(other.width_)
        , height_(other.height_)
    {
        other.image_  = VK_NULL_HANDLE;
        other.view_   = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
    }

    DepthBuffer& DepthBuffer::operator=(DepthBuffer&& other) noexcept
    {
        if (this != &other)
        {
            // Cleanup existing
            if (view_ != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device_->device(), view_, nullptr);
            }
            if (image_ != VK_NULL_HANDLE)
            {
                vkDestroyImage(device_->device(), image_, nullptr);
            }
            if (memory_ != VK_NULL_HANDLE)
            {
                vkFreeMemory(device_->device(), memory_, nullptr);
            }

            // Move
            device_ = std::move(other.device_);
            image_  = other.image_;
            view_   = other.view_;
            memory_ = other.memory_;
            format_ = other.format_;
            width_  = other.width_;
            height_ = other.height_;

            other.image_  = VK_NULL_HANDLE;
            other.view_   = VK_NULL_HANDLE;
            other.memory_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    VkFormat DepthBuffer::find_depth_format(std::shared_ptr<DeviceManager> device)
    {
        // Try depth formats in order of preference
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device->physical_device(), format, &props);

            // Check if format supports depth stencil attachment
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                return format;
            }
        }

        // Fallback to first format
        return candidates[0];
    }

    void DepthBuffer::create_image()
    {
        VkImageCreateInfo image_info{};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.extent.width  = width_;
        image_info.extent.height = height_;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.format        = format_;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateImage(device_->device(), &image_info, nullptr, &image_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create depth image", __FILE__, __LINE__);
        }

        // Get memory requirements
        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(device_->device(), image_, &mem_requirements);

        // Allocate memory
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_requirements.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                               mem_requirements.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(device_->device(), &alloc_info, nullptr, &memory_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to allocate depth image memory", __FILE__, __LINE__);
        }

        // Bind memory to image
        vkBindImageMemory(device_->device(), image_, memory_, 0);
    }

    void DepthBuffer::create_view()
    {
        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = image_;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = format_;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        VkResult result = vkCreateImageView(device_->device(), &view_info, nullptr, &view_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create depth image view", __FILE__, __LINE__);
        }
    }
} // namespace vulkan_engine::vulkan
