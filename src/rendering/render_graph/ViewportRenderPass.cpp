#include "rendering/render_graph/ViewportRenderPass.hpp"
#include "rendering/resources/RenderTarget.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    ViewportRenderPass::ViewportRenderPass(const Config& config)
        : name_(config.name)
        , config_(config)
        , render_target_(config.render_target)
    {
        if (render_target_)
        {
            create_render_pass();
            create_framebuffer();
        }
    }

    void ViewportRenderPass::setup(RenderGraphBuilder& builder)
    {
        // 声明资源依赖
        if (render_target_)
        {
            // 输出颜色附件
            // 注意：实际 handle 需要在 RenderGraph 中管理
            (void)builder; // 暂时不使用
        }
    }

    void ViewportRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        (void)ctx;

        if (!render_target_ || render_pass_ == VK_NULL_HANDLE || framebuffer_ == VK_NULL_HANDLE)
        {
            logger::error("ViewportRenderPass: Resources not initialized");
            return;
        }

        // 准备 clear values
        std::vector<VkClearValue> clear_values;
        if (config_.clear_color && render_target_->has_color())
        {
            VkClearValue color_clear{};
            color_clear.color = config_.clear_color_value;
            clear_values.push_back(color_clear);
        }
        if (config_.clear_depth && render_target_->has_depth())
        {
            VkClearValue depth_clear{};
            depth_clear.depthStencil = {config_.clear_depth_value, config_.clear_stencil_value};
            clear_values.push_back(depth_clear);
        }

        // Begin render pass
        VkRect2D render_area{};
        render_area.offset = {0, 0};
        render_area.extent = render_target_->extent();

        cmd.begin_render_pass(
                              render_pass_,
                              framebuffer_,
                              render_area,
                              clear_values
                             );

        // 设置 viewport 和 scissor
        cmd.set_viewport(
                         0.0f,
                         0.0f,
                         static_cast<float>(render_target_->width()),
                         static_cast<float>(render_target_->height()),
                         0.0f,
                         1.0f
                        );

        cmd.set_scissor(
                        0,
                        0,
                        render_target_->width(),
                        render_target_->height()
                       );

        // 执行子通道
        for (auto& pass : sub_passes_)
        {
            if (pass)
            {
                pass->execute(cmd, ctx);
            }
        }

        // End render pass
        cmd.end_render_pass();
    }

    std::vector<ImageHandle> ViewportRenderPass::get_image_outputs() const
    {
        // 返回渲染目标的图像句柄
        // 注意：这里需要与 RenderGraph 的资源系统整合
        return {};
    }

    void ViewportRenderPass::add_sub_pass(std::unique_ptr<RenderPassBase> pass)
    {
        if (pass)
        {
            sub_passes_.push_back(std::move(pass));
        }
    }

    void ViewportRenderPass::recreate_framebuffer()
    {
        cleanup_framebuffer();

        if (render_target_)
        {
            create_framebuffer();
        }
    }

    void ViewportRenderPass::create_render_pass()
    {
        if (!render_target_ || !render_target_->device())
            return;

        VkDevice device = render_target_->device()->device();

        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference>   color_refs;
        VkAttachmentReference                depth_ref{};
        bool                                 has_depth = false;

        // 颜色附件
        if (render_target_->has_color())
        {
            VkAttachmentDescription color_attachment{};
            color_attachment.format         = render_target_->color_format();
            color_attachment.samples        = render_target_->samples();
            color_attachment.loadOp         = config_.clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            attachments.push_back(color_attachment);

            VkAttachmentReference ref{};
            ref.attachment = static_cast<uint32_t>(attachments.size()) - 1;
            ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_refs.push_back(ref);
        }

        // 深度附件
        if (render_target_->has_depth())
        {
            VkAttachmentDescription depth_attachment{};
            depth_attachment.format         = render_target_->depth_format();
            depth_attachment.samples        = render_target_->samples();
            depth_attachment.loadOp         = config_.clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachments.push_back(depth_attachment);

            depth_ref.attachment = static_cast<uint32_t>(attachments.size()) - 1;
            depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            has_depth            = true;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = static_cast<uint32_t>(color_refs.size());
        subpass.pColorAttachments       = color_refs.empty() ? nullptr : color_refs.data();
        subpass.pDepthStencilAttachment = has_depth ? &depth_ref : nullptr;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments    = attachments.data();
        render_pass_info.subpassCount    = 1;
        render_pass_info.pSubpasses      = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies   = &dependency;

        VK_CHECK(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass_));
    }

    void ViewportRenderPass::create_framebuffer()
    {
        if (!render_target_ || !render_target_->device() || render_pass_ == VK_NULL_HANDLE)
            return;

        VkDevice device = render_target_->device()->device();

        std::vector<VkImageView> attachments;
        if (render_target_->has_color())
        {
            attachments.push_back(render_target_->color_image_view());
        }
        if (render_target_->has_depth())
        {
            attachments.push_back(render_target_->depth_image_view());
        }

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = render_pass_;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments    = attachments.data();
        framebuffer_info.width           = render_target_->width();
        framebuffer_info.height          = render_target_->height();
        framebuffer_info.layers          = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer_));
    }

    void ViewportRenderPass::cleanup_framebuffer()
    {
        if (!render_target_ || !render_target_->device())
            return;

        VkDevice device = render_target_->device()->device();
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
    }
} // namespace vulkan_engine::rendering