#include "rendering/resources/RenderTarget.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    RenderTarget::RenderTarget() = default;

    RenderTarget::~RenderTarget()
    {
        cleanup();
    }

    RenderTarget::RenderTarget(RenderTarget&& other) noexcept
        : device_(std::move(other.device_))
        , width_(other.width_)
        , height_(other.height_)
        , color_format_(other.color_format_)
        , depth_format_(other.depth_format_)
        , samples_(other.samples_)
        , create_color_(other.create_color_)
        , create_depth_(other.create_depth_)
        , color_image_(other.color_image_)
        , color_memory_(other.color_memory_)
        , color_image_view_(other.color_image_view_)
        , depth_image_(other.depth_image_)
        , depth_memory_(other.depth_memory_)
        , depth_image_view_(other.depth_image_view_)
    {
        other.color_image_      = VK_NULL_HANDLE;
        other.color_memory_     = VK_NULL_HANDLE;
        other.color_image_view_ = VK_NULL_HANDLE;
        other.depth_image_      = VK_NULL_HANDLE;
        other.depth_memory_     = VK_NULL_HANDLE;
        other.depth_image_view_ = VK_NULL_HANDLE;
    }

    RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            device_           = std::move(other.device_);
            width_            = other.width_;
            height_           = other.height_;
            color_format_     = other.color_format_;
            depth_format_     = other.depth_format_;
            samples_          = other.samples_;
            create_color_     = other.create_color_;
            create_depth_     = other.create_depth_;
            color_image_      = other.color_image_;
            color_memory_     = other.color_memory_;
            color_image_view_ = other.color_image_view_;
            depth_image_      = other.depth_image_;
            depth_memory_     = other.depth_memory_;
            depth_image_view_ = other.depth_image_view_;

            other.color_image_      = VK_NULL_HANDLE;
            other.color_memory_     = VK_NULL_HANDLE;
            other.color_image_view_ = VK_NULL_HANDLE;
            other.depth_image_      = VK_NULL_HANDLE;
            other.depth_memory_     = VK_NULL_HANDLE;
            other.depth_image_view_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void RenderTarget::initialize(std::shared_ptr<vulkan::DeviceManager> device, const CreateInfo& info)
    {
        device_       = device;
        width_        = info.width;
        height_       = info.height;
        color_format_ = info.color_format;
        depth_format_ = info.depth_format;
        samples_      = info.samples;
        create_color_ = info.create_color;
        create_depth_ = info.create_depth;

        create_images();
        transition_image_layout();

        logger::info("RenderTarget created: " + std::to_string(width_) + "x" + std::to_string(height_));
    }

    void RenderTarget::cleanup()
    {
        if (!device_)
            return;

        VkDevice device = device_->device();
        vkDeviceWaitIdle(device);

        // 首先销毁 Framebuffer（因为它依赖 ImageView）
        destroy_framebuffer();

        if (color_image_view_ != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, color_image_view_, nullptr);
            color_image_view_ = VK_NULL_HANDLE;
        }
        if (color_image_ != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, color_image_, nullptr);
            color_image_ = VK_NULL_HANDLE;
        }
        if (color_memory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, color_memory_, nullptr);
            color_memory_ = VK_NULL_HANDLE;
        }

        if (depth_image_view_ != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, depth_image_view_, nullptr);
            depth_image_view_ = VK_NULL_HANDLE;
        }
        if (depth_image_ != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, depth_image_, nullptr);
            depth_image_ = VK_NULL_HANDLE;
        }
        if (depth_memory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, depth_memory_, nullptr);
            depth_memory_ = VK_NULL_HANDLE;
        }
    }

    void RenderTarget::resize(uint32_t width, uint32_t height)
    {
        if (width == width_ && height == height_)
            return;

        // 先销毁 Framebuffer（因为它依赖旧的 ImageView）
        destroy_framebuffer();

        cleanup();
        width_  = width;
        height_ = height;
        create_images();
        transition_image_layout();

        logger::info("RenderTarget resized to: " + std::to_string(width_) + "x" + std::to_string(height_));
    }

    void RenderTarget::create_images()
    {
        if (create_color_)
        {
            create_color_image();
        }
        if (create_depth_)
        {
            create_depth_image();
        }
    }

    void RenderTarget::create_color_image()
    {
        VkDevice device = device_->device();

        VkImageCreateInfo image_info{};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = color_format_;
        image_info.extent.width  = width_;
        image_info.extent.height = height_;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = samples_;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(device, &image_info, nullptr, &color_image_));

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device, color_image_, &mem_reqs);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_reqs.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                               mem_reqs.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &color_memory_));
        VK_CHECK(vkBindImageMemory(device, color_image_, color_memory_, 0));

        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = color_image_;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = color_format_;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &color_image_view_));
    }

    void RenderTarget::create_depth_image()
    {
        VkDevice device = device_->device();

        VkImageCreateInfo image_info{};
        image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType     = VK_IMAGE_TYPE_2D;
        image_info.format        = depth_format_;
        image_info.extent.width  = width_;
        image_info.extent.height = height_;
        image_info.extent.depth  = 1;
        image_info.mipLevels     = 1;
        image_info.arrayLayers   = 1;
        image_info.samples       = samples_;
        image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(device, &image_info, nullptr, &depth_image_));

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device, depth_image_, &mem_reqs);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_reqs.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                               mem_reqs.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &depth_memory_));
        VK_CHECK(vkBindImageMemory(device, depth_image_, depth_memory_, 0));

        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = depth_image_;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = depth_format_;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &depth_image_view_));
    }

    void RenderTarget::transition_image_layout()
    {
        // 使用一次性命令缓冲将图像从 UNDEFINED 转换到适当布局
        VkDevice device = device_->device();

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = device_->graphics_queue_family();
        pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkCommandPool cmd_pool = VK_NULL_HANDLE;
        VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool));

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = cmd_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
        VK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer));

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &begin_info));

        // 颜色附件：UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        if (color_image_ != VK_NULL_HANDLE)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = color_image_;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = 0;
            barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                                 cmd_buffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier
                                );
        }

        // 深度附件：UNDEFINED -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        if (depth_image_ != VK_NULL_HANDLE)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = depth_image_;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = 0;
            barrier.dstAccessMask                   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                                 cmd_buffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier
                                );
        }

        VK_CHECK(vkEndCommandBuffer(cmd_buffer));

        VkSubmitInfo submit_info{};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buffer;

        VK_CHECK(vkQueueSubmit(device_->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(device_->graphics_queue()));

        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);
        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }

    void RenderTarget::create_framebuffer(VkRenderPass render_pass)
    {
        if (!device_ || render_pass == VK_NULL_HANDLE)
        {
            return;
        }

        // 销毁旧的 Framebuffer（如果存在）
        destroy_framebuffer();

        // 收集附件
        std::vector<VkImageView> attachments;
        if (has_color())
        {
            attachments.push_back(color_image_view_);
        }
        if (has_depth())
        {
            attachments.push_back(depth_image_view_);
        }

        // 创建新的 Framebuffer
        framebuffer_ = std::make_unique<vulkan::Framebuffer>(
                                                             device_,
                                                             render_pass,
                                                             attachments,
                                                             width_,
                                                             height_,
                                                             1 // layers
                                                            );
    }

    void RenderTarget::destroy_framebuffer()
    {
        // Framebuffer 是 unique_ptr，reset 会自动销毁
        framebuffer_.reset();
    }

    bool RenderTarget::has_framebuffer() const
    {
        return framebuffer_ != nullptr;
    }

    VkFramebuffer RenderTarget::framebuffer_handle() const
    {
        return framebuffer_ ? framebuffer_->handle() : VK_NULL_HANDLE;
    }
} // namespace vulkan_engine::rendering