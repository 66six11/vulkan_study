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
        , render_pass_(config.render_pass)
    {
        // Validate required parameters
        if (!render_target_)
        {
            logger::error("ViewportRenderPass: RenderTarget is required");
            return;
        }
        if (render_pass_ == VK_NULL_HANDLE)
        {
            logger::error("ViewportRenderPass: External RenderPass is required (must be provided in Config)");
            return;
        }
        // Framebuffer is managed externally, set via set_framebuffer()
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

        if (!render_target_ || render_pass_ == VK_NULL_HANDLE)
        {
            logger::error("ViewportRenderPass: RenderTarget or RenderPass not initialized");
            return;
        }

        if (framebuffer_ == VK_NULL_HANDLE)
        {
            logger::error("ViewportRenderPass: Framebuffer not set (must be set externally via set_framebuffer())");
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
        // Note: Both RenderPass and Framebuffer are managed externally
        // This method is called when external resources are recreated
        // ViewportRenderPass just updates its references via set_framebuffer()
    }

    ViewportRenderPass::~ViewportRenderPass()
    {
        // Note: RenderPass and Framebuffer are managed externally
        // Just clear our references
        render_pass_ = VK_NULL_HANDLE;
        framebuffer_ = VK_NULL_HANDLE;
    }
} // namespace vulkan_engine::rendering