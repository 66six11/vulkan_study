#include "rendering/render_graph/CubeRenderPass.hpp"
#include "core/utils/Logger.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace vulkan_engine::rendering
{
    CubeRenderPass::CubeRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void CubeRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // Declare image outputs for barrier generation
        if (config_.color_output.valid())
        {
            image_outputs_.push_back(config_.color_output);
        }
        if (config_.depth_output.valid())
        {
            image_outputs_.push_back(config_.depth_output);
        }
    }

    void CubeRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        logger::info("CubeRenderPass::execute - Starting render");

        if (!config_.vertex_buffer || !config_.index_buffer)
        {
            logger::error("CubeRenderPass: Missing required geometry resources");
            return;
        }

        auto material = config_.material_ref.lock();
        if (!material)
        {
            logger::error("CubeRenderPass: Material not available");
            return;
        }

        logger::info("CubeRenderPass: Rendering with material " + material->name());

        // Begin render pass
        VkClearValue clear_values[2];
        clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clear_values[1].depthStencil = {1.0f, 0};

        VkRect2D render_area{};
        render_area.offset = {0, 0};
        render_area.extent = {ctx.width, ctx.height};

        cmd.begin_render_pass(ctx.render_pass, ctx.framebuffer, render_area, {clear_values[0], clear_values[1]});

        // Bind material (pipeline and descriptor sets) with current render pass
        material->bind(cmd, ctx.render_pass);

        // Set viewport - 使用渲染上下文的实际尺寸，而不是固定的配置尺寸
        cmd.set_viewport(
                         0.0f,
                         0.0f,
                         static_cast<float>(ctx.width),
                         static_cast<float>(ctx.height),
                         0.0f,
                         1.0f
                        );

        // Set scissor - 使用渲染上下文的实际尺寸
        cmd.set_scissor(0, 0, ctx.width, ctx.height);

        // Push MVP matrix
        cmd.push_constants(material->pipeline_layout(),
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(glm::mat4),
                           &current_mvp_);

        // Bind vertex buffer
        cmd.bind_vertex_buffer(config_.vertex_buffer->handle(), 0);

        // Bind index buffer
        cmd.bind_index_buffer(config_.index_buffer->handle(), config_.index_type);

        // Draw indexed
        cmd.draw_indexed(config_.index_count, 1, 0, 0, 0);
        logger::info("CubeRenderPass: Draw call submitted");

        // End render pass
        cmd.end_render_pass();
        logger::info("CubeRenderPass::execute - Completed");
    }

    void CubeRenderPass::set_mvp_matrix(const glm::mat4& mvp)
    {
        current_mvp_ = mvp;
    }
} // namespace vulkan_engine::rendering