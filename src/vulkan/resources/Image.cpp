#include "vulkan/resources/Image.hpp"
#include <stdexcept>

namespace vulkan_engine::vulkan
{
    Image::Image(
        std::shared_ptr<DeviceManager> device,
        uint32_t                       width,
        uint32_t                       height,
        VkFormat                       format,
        VkImageUsageFlags              usage,
        VkMemoryPropertyFlags          properties,
        uint32_t                       mip_levels,
        uint32_t                       array_layers)
        : device_(std::move(device))
        , width_(width)
        , height_(height)
        , format_(format)
        , mip_levels_(mip_levels)
        , array_layers_(array_layers)
    {
        // Create image
        VkImageCreateInfo image_info{};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.extent.width  = width;
        image_info.extent.height = height;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = mip_levels;
        image_info.arrayLayers   = array_layers;
        image_info.format        = format;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage         = usage;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples       = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device_->device(), &image_info, nullptr, &image_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image");
        }

        // Get memory requirements
        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(device_->device(), image_, &mem_requirements);

        // Allocate memory
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_requirements.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(mem_requirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device_->device(), &alloc_info, nullptr, &memory_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate image memory");
        }

        // Bind memory to image
        vkBindImageMemory(device_->device(), image_, memory_, 0);

        // Create default image view (whole image)
        create_view(VK_IMAGE_VIEW_TYPE_2D, format, {VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, array_layers});
    }

    Image::~Image()
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

    void Image::create_view(VkImageViewType view_type, VkFormat format, const ImageSubresourceRange& range)
    {
        // Destroy old view if exists
        if (view_ != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device_->device(), view_, nullptr);
        }

        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = image_;
        view_info.viewType                        = view_type;
        view_info.format                          = format;
        view_info.subresourceRange.aspectMask     = range.aspect_mask;
        view_info.subresourceRange.baseMipLevel   = range.base_mip_level;
        view_info.subresourceRange.levelCount     = range.level_count;
        view_info.subresourceRange.baseArrayLayer = range.base_array_layer;
        view_info.subresourceRange.layerCount     = range.layer_count;

        if (vkCreateImageView(device_->device(), &view_info, nullptr, &view_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image view");
        }
    }

    void Image::transition_layout(VkCommandBuffer cmd, VkImageLayout new_layout)
    {
        if (current_layout_ == new_layout)
        {
            return;
        }

        auto info = get_transition_info(current_layout_, new_layout);

        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = current_layout_;
        barrier.newLayout                       = new_layout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image_;
        barrier.subresourceRange.aspectMask     = (format_ == VK_FORMAT_D32_SFLOAT || format_ == VK_FORMAT_D32_SFLOAT_S8_UINT || format_ == VK_FORMAT_D24_UNORM_S8_UINT)
                                                      ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                      : VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = mip_levels_;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = array_layers_;
        barrier.srcAccessMask                   = info.src_access_mask;
        barrier.dstAccessMask                   = info.dst_access_mask;

        vkCmdPipelineBarrier(
            cmd,
            info.src_stage_mask,
            info.dst_stage_mask,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        current_layout_ = new_layout;
    }

    Image::TransitionInfo Image::get_transition_info(VkImageLayout old_layout, VkImageLayout new_layout)
    {
        TransitionInfo info{};

        // Source stage and access
        switch (old_layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                info.src_access_mask = 0;
                info.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                info.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                info.src_access_mask = VK_ACCESS_TRANSFER_READ_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                info.src_access_mask = VK_ACCESS_SHADER_READ_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                info.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                info.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                info.src_access_mask = VK_ACCESS_MEMORY_READ_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            default:
                info.src_access_mask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                info.src_stage_mask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                break;
        }

        // Destination stage and access
        switch (new_layout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                info.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                info.dst_access_mask = VK_ACCESS_TRANSFER_READ_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                info.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                info.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                info.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                info.dst_access_mask = 0;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            default:
                info.dst_access_mask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                info.dst_stage_mask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                break;
        }

        return info;
    }

    void Image::generate_mipmaps()
    {
        // Placeholder: Mipmap generation would be done via command buffer
    }

    void Image::upload_data(const void* /*data*/, VkDeviceSize /*size*/)
    {
        // Placeholder: Image upload would use staging buffer and command buffer
    }

    void Image::download_data(void* /*data*/, VkDeviceSize /*size*/)
    {
        // Placeholder: Image download would use staging buffer and command buffer
    }

    // ImageBuilder implementation
    ImageBuilder::ImageBuilder(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    ImageBuilder& ImageBuilder::width(uint32_t width)
    {
        width_ = width;
        return *this;
    }

    ImageBuilder& ImageBuilder::height(uint32_t height)
    {
        height_ = height;
        return *this;
    }

    ImageBuilder& ImageBuilder::depth(uint32_t depth)
    {
        depth_ = depth;
        return *this;
    }

    ImageBuilder& ImageBuilder::format(VkFormat format)
    {
        format_ = format;
        return *this;
    }

    ImageBuilder& ImageBuilder::usage(VkImageUsageFlags usage)
    {
        usage_ = usage;
        return *this;
    }

    ImageBuilder& ImageBuilder::mip_levels(uint32_t levels)
    {
        mip_levels_ = levels;
        return *this;
    }

    ImageBuilder& ImageBuilder::array_layers(uint32_t layers)
    {
        array_layers_ = layers;
        return *this;
    }

    ImageBuilder& ImageBuilder::samples(VkSampleCountFlagBits samples)
    {
        samples_ = samples;
        return *this;
    }

    ImageBuilder& ImageBuilder::device_local(bool local)
    {
        if (local)
        {
            properties_ |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        else
        {
            properties_ &= ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        return *this;
    }

    std::unique_ptr<Image> ImageBuilder::build()
    {
        return std::make_unique < Image > (device_, width_, height_, format_, usage_, properties_, mip_levels_, array_layers_);
    }

    // ImageManager implementation
    ImageManager::ImageManager(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    ImageManager::~ImageManager() = default;

    std::shared_ptr<Image> ImageManager::create_image(
        uint32_t              width,
        uint32_t              height,
        VkFormat              format,
        VkImageUsageFlags     usage,
        VkMemoryPropertyFlags properties)
    {
        return std::make_shared < Image > (device_, width, height, format, usage, properties);
    }

    std::shared_ptr<Image> ImageManager::create_color_attachment(uint32_t width, uint32_t height, VkFormat format)
    {
        return create_image(width,
                            height,
                            format,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    std::shared_ptr<Image> ImageManager::create_depth_attachment(uint32_t width, uint32_t height, VkFormat format)
    {
        return create_image(width,
                            height,
                            format,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    std::shared_ptr<Image> ImageManager::create_texture(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        uint32_t mip_levels)
    {
        auto image = std::make_shared < Image > (device_, width, height, format,
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                 mip_levels, 1);
        return image;
    }

    void ImageManager::destroy_image(std::shared_ptr<Image> image)
    {
        image.reset();
    }

    ImageManager::Stats ImageManager::get_stats() const
    {
        return Stats{}; // Placeholder
    }
} // namespace vulkan_engine::vulkan