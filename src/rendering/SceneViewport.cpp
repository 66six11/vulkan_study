#include "rendering/SceneViewport.hpp"
#include "core/utils/Logger.hpp"
#include "vulkan/utils/VulkanError.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace vulkan_engine::rendering
{
    SceneViewport::SceneViewport() = default;

    SceneViewport::~SceneViewport()
    {
        cleanup();

        // Destroy ImGui sampler
        if (imgui_sampler_ != VK_NULL_HANDLE && device_)
        {
            vkDestroySampler(device_->device(), imgui_sampler_, nullptr);
            imgui_sampler_ = VK_NULL_HANDLE;
        }
    }

    SceneViewport::SceneViewport(SceneViewport&& other) noexcept
        : device_(std::move(other.device_))
        , width_(other.width_)
        , height_(other.height_)
        , color_format_(other.color_format_)
        , depth_format_(other.depth_format_)
        , samples_(other.samples_)
        , color_image_(other.color_image_)
        , color_memory_(other.color_memory_)
        , color_image_view_(other.color_image_view_)
        , depth_image_(other.depth_image_)
        , depth_memory_(other.depth_memory_)
        , depth_image_view_(other.depth_image_view_)
        , render_pass_(other.render_pass_)
        , framebuffer_(other.framebuffer_)
        , imgui_descriptor_set_(other.imgui_descriptor_set_)
        , imgui_sampler_(other.imgui_sampler_)
    {
        other.color_image_          = VK_NULL_HANDLE;
        other.color_memory_         = VK_NULL_HANDLE;
        other.color_image_view_     = VK_NULL_HANDLE;
        other.depth_image_          = VK_NULL_HANDLE;
        other.depth_memory_         = VK_NULL_HANDLE;
        other.depth_image_view_     = VK_NULL_HANDLE;
        other.render_pass_          = VK_NULL_HANDLE;
        other.framebuffer_          = VK_NULL_HANDLE;
        other.imgui_descriptor_set_ = VK_NULL_HANDLE;
        other.imgui_sampler_        = VK_NULL_HANDLE;
    }

    SceneViewport& SceneViewport::operator=(SceneViewport&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            device_               = std::move(other.device_);
            width_                = other.width_;
            height_               = other.height_;
            color_format_         = other.color_format_;
            depth_format_         = other.depth_format_;
            samples_              = other.samples_;
            color_image_          = other.color_image_;
            color_memory_         = other.color_memory_;
            color_image_view_     = other.color_image_view_;
            depth_image_          = other.depth_image_;
            depth_memory_         = other.depth_memory_;
            depth_image_view_     = other.depth_image_view_;
            render_pass_          = other.render_pass_;
            framebuffer_          = other.framebuffer_;
            imgui_descriptor_set_ = other.imgui_descriptor_set_;
            imgui_sampler_        = other.imgui_sampler_;

            other.color_image_          = VK_NULL_HANDLE;
            other.color_memory_         = VK_NULL_HANDLE;
            other.color_image_view_     = VK_NULL_HANDLE;
            other.depth_image_          = VK_NULL_HANDLE;
            other.depth_memory_         = VK_NULL_HANDLE;
            other.depth_image_view_     = VK_NULL_HANDLE;
            other.render_pass_          = VK_NULL_HANDLE;
            other.framebuffer_          = VK_NULL_HANDLE;
            other.imgui_descriptor_set_ = VK_NULL_HANDLE;
            other.imgui_sampler_        = VK_NULL_HANDLE;
        }
        return *this;
    }

    void SceneViewport::initialize(std::shared_ptr<vulkan::DeviceManager> device, const CreateInfo& info)
    {
        device_       = device;
        width_        = info.width;
        height_       = info.height;
        color_format_ = info.color_format;
        depth_format_ = info.depth_format;
        samples_      = info.samples;

        create_images();
        create_render_pass();
        create_framebuffer();

        // Initial render pass to transition image layout from UNDEFINED to SHADER_READ_ONLY_OPTIMAL
        transition_image_layout();

        logger::info("SceneViewport created: " + std::to_string(width_) + "x" + std::to_string(height_));
    }

    void SceneViewport::transition_image_layout()
    {
        VkDevice device = device_->device();

        // Create temporary command pool and buffer
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex        = device_->graphics_queue_family();
        pool_info.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkCommandPool cmd_pool = VK_NULL_HANDLE;
        vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool);

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = cmd_pool;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = 1;

        VkCommandBuffer cmd_buffer = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer);

        // Begin command buffer
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd_buffer, &begin_info);

        // Begin render pass (transitions from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL)
        VkClearValue clear_values[2];
        clear_values[0].color        = {0.0f, 0.0f, 0.0f, 1.0f};
        clear_values[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass            = render_pass_;
        render_pass_info.framebuffer           = framebuffer_;
        render_pass_info.renderArea.offset     = {0, 0};
        render_pass_info.renderArea.extent     = {width_, height_};
        render_pass_info.clearValueCount       = 2;
        render_pass_info.pClearValues          = clear_values;

        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(cmd_buffer); // Transitions to SHADER_READ_ONLY_OPTIMAL

        // End command buffer
        vkEndCommandBuffer(cmd_buffer);

        // Submit and wait
        VkSubmitInfo submit_info       = {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buffer;

        vkQueueSubmit(device_->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(device_->graphics_queue());

        // Cleanup
        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);
        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }

    void SceneViewport::cleanup()
    {
        if (!device_)
        {
            return;
        }

        VkDevice device = device_->device();
        vkDeviceWaitIdle(device);

        if (framebuffer_ != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer_, nullptr);
            framebuffer_ = VK_NULL_HANDLE;
        }

        if (render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device, render_pass_, nullptr);
            render_pass_ = VK_NULL_HANDLE;
        }

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

        imgui_descriptor_set_ = VK_NULL_HANDLE;
        // Note: imgui_sampler_ is not destroyed here to allow reuse during resize
    }

    void SceneViewport::resize(uint32_t width, uint32_t height)
    {
        if (width == width_ && height == height_)
        {
            return;
        }

        cleanup();
        width_  = width;
        height_ = height;
        create_images();
        create_render_pass();
        create_framebuffer();

        // Recreate ImGui descriptor set (but keep the sampler)
        imgui_descriptor_set_ = VK_NULL_HANDLE;

        // Transition new image layout from UNDEFINED to SHADER_READ_ONLY_OPTIMAL
        transition_image_layout();

        logger::info("SceneViewport resized: " + std::to_string(width_) + "x" + std::to_string(height_));
    }

    ImTextureID SceneViewport::imgui_texture_id() const
    {
        // Create sampler on first use
        if (imgui_sampler_ == VK_NULL_HANDLE)
        {
            VkSamplerCreateInfo sampler_info     = {};
            sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter               = VK_FILTER_LINEAR;
            sampler_info.minFilter               = VK_FILTER_LINEAR;
            sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.anisotropyEnable        = VK_FALSE;
            sampler_info.maxAnisotropy           = 1.0f;
            sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            sampler_info.unnormalizedCoordinates = VK_FALSE;
            sampler_info.compareEnable           = VK_FALSE;
            sampler_info.mipLodBias              = 0.0f;
            sampler_info.minLod                  = 0.0f;
            sampler_info.maxLod                  = 1.0f;

            vkCreateSampler(device_->device(),
                            &sampler_info,
                            nullptr,
                            &const_cast<SceneViewport*>(this)->imgui_sampler_);
        }

        // Create descriptor set on first use or after resize
        if (imgui_descriptor_set_ == VK_NULL_HANDLE && color_image_view_ != VK_NULL_HANDLE && imgui_sampler_ != VK_NULL_HANDLE)
        {
            const_cast<SceneViewport*>(this)->imgui_descriptor_set_ =
                    ImGui_ImplVulkan_AddTexture(
                                                imgui_sampler_,
                                                color_image_view_,
                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        return reinterpret_cast<ImTextureID>(imgui_descriptor_set_);
    }

    void SceneViewport::begin_render_pass(VkCommandBuffer command_buffer)
    {
        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass            = render_pass_;
        render_pass_info.framebuffer           = framebuffer_;
        render_pass_info.renderArea.offset     = {0, 0};
        render_pass_info.renderArea.extent     = {width_, height_};

        VkClearValue clear_values[2] = {};
        clear_values[0].color        = {0.1f, 0.1f, 0.1f, 1.0f}; // Dark gray background
        clear_values[1].depthStencil = {1.0f, 0};

        render_pass_info.clearValueCount = 2;
        render_pass_info.pClearValues    = clear_values;

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = static_cast<float>(width_);
        viewport.height     = static_cast<float>(height_);
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset   = {0, 0};
        scissor.extent   = {width_, height_};
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }

    void SceneViewport::end_render_pass(VkCommandBuffer command_buffer)
    {
        vkCmdEndRenderPass(command_buffer);
    }

    void SceneViewport::create_images()
    {
        VkDevice device = device_->device();

        // Create color image
        VkImageCreateInfo color_image_info = {};
        color_image_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        color_image_info.imageType         = VK_IMAGE_TYPE_2D;
        color_image_info.format            = color_format_;
        color_image_info.extent.width      = width_;
        color_image_info.extent.height     = height_;
        color_image_info.extent.depth      = 1;
        color_image_info.mipLevels         = 1;
        color_image_info.arrayLayers       = 1;
        color_image_info.samples           = samples_;
        color_image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
        color_image_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        color_image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        color_image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(device, &color_image_info, nullptr, &color_image_));

        VkMemoryRequirements color_mem_reqs;
        vkGetImageMemoryRequirements(device, color_image_, &color_mem_reqs);

        VkMemoryAllocateInfo color_alloc_info = {};
        color_alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        color_alloc_info.allocationSize       = color_mem_reqs.size;
        color_alloc_info.memoryTypeIndex      = device_->find_memory_type(
                                                                          color_mem_reqs.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(device, &color_alloc_info, nullptr, &color_memory_));
        VK_CHECK(vkBindImageMemory(device, color_image_, color_memory_, 0));

        // Create color image view
        VkImageViewCreateInfo color_view_info           = {};
        color_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_view_info.image                           = color_image_;
        color_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        color_view_info.format                          = color_format_;
        color_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        color_view_info.subresourceRange.baseMipLevel   = 0;
        color_view_info.subresourceRange.levelCount     = 1;
        color_view_info.subresourceRange.baseArrayLayer = 0;
        color_view_info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &color_view_info, nullptr, &color_image_view_));

        // Create depth image
        VkImageCreateInfo depth_image_info = {};
        depth_image_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depth_image_info.imageType         = VK_IMAGE_TYPE_2D;
        depth_image_info.format            = depth_format_;
        depth_image_info.extent.width      = width_;
        depth_image_info.extent.height     = height_;
        depth_image_info.extent.depth      = 1;
        depth_image_info.mipLevels         = 1;
        depth_image_info.arrayLayers       = 1;
        depth_image_info.samples           = samples_;
        depth_image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
        depth_image_info.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depth_image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        depth_image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(device, &depth_image_info, nullptr, &depth_image_));

        VkMemoryRequirements depth_mem_reqs;
        vkGetImageMemoryRequirements(device, depth_image_, &depth_mem_reqs);

        VkMemoryAllocateInfo depth_alloc_info = {};
        depth_alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        depth_alloc_info.allocationSize       = depth_mem_reqs.size;
        depth_alloc_info.memoryTypeIndex      = device_->find_memory_type(
                                                                          depth_mem_reqs.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(device, &depth_alloc_info, nullptr, &depth_memory_));
        VK_CHECK(vkBindImageMemory(device, depth_image_, depth_memory_, 0));

        // Create depth image view
        VkImageViewCreateInfo depth_view_info           = {};
        depth_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depth_view_info.image                           = depth_image_;
        depth_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        depth_view_info.format                          = depth_format_;
        depth_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_view_info.subresourceRange.baseMipLevel   = 0;
        depth_view_info.subresourceRange.levelCount     = 1;
        depth_view_info.subresourceRange.baseArrayLayer = 0;
        depth_view_info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &depth_view_info, nullptr, &depth_image_view_));
    }

    void SceneViewport::create_render_pass()
    {
        VkDevice device = device_->device();

        VkAttachmentDescription attachments[2] = {};

        // Color attachment
        attachments[0].format         = color_format_;
        attachments[0].samples        = samples_;
        attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Depth attachment
        attachments[1].format         = depth_format_;
        attachments[1].samples        = samples_;
        attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref = {};
        color_ref.attachment            = 0;
        color_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_ref = {};
        depth_ref.attachment            = 1;
        depth_ref.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount        = 2;
        render_pass_info.pAttachments           = attachments;
        render_pass_info.subpassCount           = 1;
        render_pass_info.pSubpasses             = &subpass;

        VK_CHECK(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass_));
    }

    void SceneViewport::create_framebuffer()
    {
        VkDevice device = device_->device();

        VkImageView attachments[2] = {color_image_view_, depth_image_view_};

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass              = render_pass_;
        framebuffer_info.attachmentCount         = 2;
        framebuffer_info.pAttachments            = attachments;
        framebuffer_info.width                   = width_;
        framebuffer_info.height                  = height_;
        framebuffer_info.layers                  = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer_));
    }
} // namespace vulkan_engine::rendering
