#include "rendering/render_graph/RenderGraphPass.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    // ============================================================================
    // RenderPassBase
    // ============================================================================

    void RenderPassBase::execute(CommandBuffer& /*cmd*/)
    {
        // Base implementation - should be overridden
        logger::warn("RenderPassBase::execute() called directly - should use execute(cmd, ctx)");
    }

    // ============================================================================
    // ClearRenderPass
    // ============================================================================

    ClearRenderPass::ClearRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void ClearRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // Declare output resources for barrier generation
        if (config_.color_output.valid())
        {
            image_outputs_.push_back(config_.color_output);
        }
        if (config_.depth_output.valid())
        {
            image_outputs_.push_back(config_.depth_output);
        }
    }

    void ClearRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        // Prepare clear values
        std::vector<VkClearValue> clear_values;

        if (config_.enable_color_clear)
        {
            VkClearValue color_clear{};
            color_clear.color = {
                {
                    config_.color[0],
                    config_.color[1],
                    config_.color[2],
                    config_.color[3]
                }
            };
            clear_values.push_back(color_clear);
        }

        if (config_.enable_depth_clear)
        {
            VkClearValue depth_clear{};
            depth_clear.depthStencil = {config_.depth, config_.stencil};
            clear_values.push_back(depth_clear);
        }

        // Begin render pass with clear
        VkRect2D render_area{};
        render_area.offset = {0, 0};
        render_area.extent = {ctx.width, ctx.height};

        cmd.begin_render_pass(ctx.render_pass, ctx.framebuffer, render_area, clear_values);
        cmd.end_render_pass();
    }

    // ============================================================================
    // GeometryRenderPass
    // ============================================================================

    GeometryRenderPass::GeometryRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void GeometryRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // Geometry pass reads vertex/index buffers and writes to color/depth attachments
        // In a more complete implementation, we would declare these resources
    }

    void GeometryRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        if (config_.meshes.empty())
        {
            return;
        }

        // Calculate actual viewport dimensions
        float vp_width = config_.viewport_width <= 1.0f
                             ? config_.viewport_width * static_cast<float>(ctx.width)
                             : config_.viewport_width;
        float vp_height = config_.viewport_height <= 1.0f
                              ? config_.viewport_height * static_cast<float>(ctx.height)
                              : config_.viewport_height;

        // Render each mesh
        for (const auto& mesh : config_.meshes)
        {
            if (!mesh.pipeline || !mesh.vertex_buffer)
            {
                continue;
            }

            // Bind pipeline
            cmd.bind_graphics_pipeline(*mesh.pipeline);

            // Set viewport and scissor
            cmd.set_viewport(
                             config_.viewport_x,
                             config_.viewport_y,
                             vp_width,
                             vp_height,
                             0.0f,
                             1.0f);

            cmd.set_scissor(0, 0, ctx.width, ctx.height);

            // Bind descriptor set if provided
            if (mesh.pipeline_layout != VK_NULL_HANDLE && mesh.descriptor_set != VK_NULL_HANDLE)
            {
                std::vector<VkDescriptorSet> sets = {mesh.descriptor_set};
                cmd.bind_descriptor_sets(mesh.pipeline_layout, 0, sets);
            }

            // Bind vertex buffer
            cmd.bind_vertex_buffer(mesh.vertex_buffer->handle(), 0);

            // Bind index buffer and draw, or draw without indices
            if (mesh.index_buffer && mesh.index_count > 0)
            {
                cmd.bind_index_buffer(mesh.index_buffer->handle(), mesh.index_type);
                cmd.draw_indexed(mesh.index_count, 1, 0, mesh.vertex_offset, 0);
            }
            else
            {
                cmd.draw(mesh.index_count, 1, 0, 0);
            }
        }
    }

    void GeometryRenderPass::add_mesh(const MeshDraw& mesh)
    {
        config_.meshes.push_back(mesh);
    }

    void GeometryRenderPass::clear_meshes()
    {
        config_.meshes.clear();
    }

    // ============================================================================
    // PresentRenderPass
    // ============================================================================

    PresentRenderPass::PresentRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void PresentRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // Present pass reads the rendered image and transitions it for presentation
        if (config_.source_image.valid())
        {
            // builder.read(config_.source_image);
        }
    }

    void PresentRenderPass::execute(vulkan::RenderCommandBuffer& /*cmd*/, const RenderContext& /*ctx*/)
    {
        // Present pass typically doesn't record commands
        // The transition to PRESENT_SRC_KHR is handled by the render pass end
        // or by an explicit barrier in more complex scenarios
    }

    // ============================================================================
    // UIRenderPass
    // ============================================================================

    UIRenderPass::UIRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void UIRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // UI pass would declare its resources here
    }

    void UIRenderPass::execute(vulkan::RenderCommandBuffer& /*cmd*/, const RenderContext& /*ctx*/)
    {
        // Placeholder for UI rendering
        // This would integrate with ImGui or other UI library
        logger::debug("UIRenderPass::execute() - placeholder");
    }
} // namespace vulkan_engine::rendering