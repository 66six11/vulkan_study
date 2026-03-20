#include "engine/rendering/resources/RenderTarget.hpp"
#include "engine/rhi/vulkan/resources/Framebuffer.hpp"
#include "engine/rhi/vulkan/memory/VmaImage.hpp"
#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"
#include "engine/core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    RenderTarget::RenderTarget() = default;

    RenderTarget::~RenderTarget()
    {
        cleanup();
    }

    RenderTarget::RenderTarget(RenderTarget&& other) noexcept
        : allocator_(std::move(other.allocator_))
        , width_(other.width_)
        , height_(other.height_)
        , color_format_(other.color_format_)
        , depth_format_(other.depth_format_)
        , samples_(other.samples_)
        , create_color_(other.create_color_)
        , create_depth_(other.create_depth_)
        , color_image_(std::move(other.color_image_))
        , color_image_view_(other.color_image_view_)
        , depth_image_(std::move(other.depth_image_))
        , depth_image_view_(other.depth_image_view_)
        , framebuffer_(std::move(other.framebuffer_))
    {
        other.color_image_view_ = VK_NULL_HANDLE;
        other.depth_image_view_ = VK_NULL_HANDLE;
    }

    RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            allocator_        = std::move(other.allocator_);
            width_            = other.width_;
            height_           = other.height_;
            color_format_     = other.color_format_;
            depth_format_     = other.depth_format_;
            samples_          = other.samples_;
            create_color_     = other.create_color_;
            create_depth_     = other.create_depth_;
            color_image_      = std::move(other.color_image_);
            color_image_view_ = other.color_image_view_;
            depth_image_      = std::move(other.depth_image_);
            depth_image_view_ = other.depth_image_view_;
            framebuffer_      = std::move(other.framebuffer_);

            other.color_image_view_ = VK_NULL_HANDLE;
            other.depth_image_view_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void RenderTarget::initialize(std::shared_ptr<vulkan::memory::VmaAllocator> allocator, const CreateInfo& info)
    {
        allocator_    = allocator;
        width_        = info.width;
        height_       = info.height;
        color_format_ = info.color_format;
        depth_format_ = info.depth_format;
        samples_      = info.samples;
        create_color_ = info.create_color;
        create_depth_ = info.create_depth;

        create_images();
        transition_image_layout();

        LOG_INFO("RenderTarget created: " + std::to_string(width_) + "x" + std::to_string(height_));
    }

    void RenderTarget::cleanup()
    {
        if (!allocator_)
            return;

        VkDevice device = allocator_->device()->device().handle();
        vkDeviceWaitIdle(device);

        // 棣栧厛閿€姣?Framebuffer锛堝洜涓哄畠渚濊禆 ImageView锛?
        destroy_framebuffer();

        // 閿€姣?ImageView锛堢敱 VmaImage 绠＄悊锛?
        if (color_image_view_ != VK_NULL_HANDLE && color_image_)
        {
            color_image_->destroyView(color_image_view_);
            color_image_view_ = VK_NULL_HANDLE;
        }
        if (depth_image_view_ != VK_NULL_HANDLE && depth_image_)
        {
            depth_image_->destroyView(depth_image_view_);
            depth_image_view_ = VK_NULL_HANDLE;
        }

        // VmaImage 浼氳嚜鍔ㄦ竻鐞?
        color_image_.reset();
        depth_image_.reset();
    }

    void RenderTarget::resize(uint32_t width, uint32_t height)
    {
        if (width == width_ && height == height_)
            return;

        // 鍏堥攢姣?Framebuffer锛堝洜涓哄畠渚濊禆鏃х殑 ImageView锛?
        destroy_framebuffer();

        cleanup();
        width_  = width;
        height_ = height;
        create_images();
        transition_image_layout();

        LOG_INFO("RenderTarget resized to: " + std::to_string(width_) + "x" + std::to_string(height_));
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
        // 浣跨敤 VmaImageBuilder 鍒涘缓棰滆壊闄勪欢
        color_image_ = vulkan::memory::VmaImageBuilder::createColorAttachment(
                                                                              allocator_,
                                                                              width_,
                                                                              height_,
                                                                              color_format_,
                                                                              1,
                                                                              // mipLevels
                                                                              samples_
                                                                             );

        // 鍒涘缓榛樿 ImageView
        color_image_view_ = color_image_->createView(
                                                     VK_IMAGE_VIEW_TYPE_2D,
                                                     color_format_,
                                                     vulkan::memory::ImageSubresourceRange::colorAll()
                                                    );
    }

    void RenderTarget::create_depth_image()
    {
        // 浣跨敤 VmaImageBuilder 鍒涘缓娣卞害闄勪欢
        depth_image_ = vulkan::memory::VmaImageBuilder::createDepthAttachment(
                                                                              allocator_,
                                                                              width_,
                                                                              height_,
                                                                              depth_format_,
                                                                              samples_
                                                                             );

        // 鍒涘缓榛樿 ImageView
        depth_image_view_ = depth_image_->createView(
                                                     VK_IMAGE_VIEW_TYPE_2D,
                                                     depth_format_,
                                                     vulkan::memory::ImageSubresourceRange::depthAll()
                                                    );
    }

    void RenderTarget::transition_image_layout()
    {
        VkDevice device = allocator_->device()->device().handle();

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = allocator_->device()->graphics_queue_family();
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

        // 浣跨敤 VmaImage 鐨?transitionLayout 鏂规硶
        if (color_image_)
        {
            color_image_->transitionLayout(
                                           cmd_buffer,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           vulkan::memory::ImageSubresourceRange::colorAll()
                                          );
        }

        if (depth_image_)
        {
            depth_image_->transitionLayout(
                                           cmd_buffer,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                           vulkan::memory::ImageSubresourceRange::depthAll()
                                          );
        }

        VK_CHECK(vkEndCommandBuffer(cmd_buffer));

        VkSubmitInfo submit_info{};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buffer;

        VK_CHECK(vkQueueSubmit(allocator_->device()->graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(allocator_->device()->graphics_queue().handle()));

        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);
        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }

    VkImageView RenderTarget::color_image_view() const
    {
        return color_image_view_;
    }

    VkImageView RenderTarget::depth_image_view() const
    {
        return depth_image_view_;
    }

    void RenderTarget::create_framebuffer(VkRenderPass render_pass)
    {
        if (!allocator_ || render_pass == VK_NULL_HANDLE)
        {
            return;
        }

        // 閿€姣佹棫鐨?Framebuffer锛堝鏋滃瓨鍦級
        destroy_framebuffer();

        // 鏀堕泦闄勪欢
        std::vector<VkImageView> attachments;
        if (has_color())
        {
            attachments.push_back(color_image_view_);
        }
        if (has_depth())
        {
            attachments.push_back(depth_image_view_);
        }

        // 鍒涘缓鏂扮殑 Framebuffer
        framebuffer_ = std::make_unique<vulkan::Framebuffer>(
                                                             allocator_->device(),
                                                             render_pass,
                                                             attachments,
                                                             width_,
                                                             height_,
                                                             1 // layers
                                                            );
    }

    void RenderTarget::destroy_framebuffer()
    {
        // Framebuffer 鏄?unique_ptr锛宺eset 浼氳嚜鍔ㄩ攢姣?
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