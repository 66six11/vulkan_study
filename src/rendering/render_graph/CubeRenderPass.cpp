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
        if (!config_.vertex_buffer || !config_.index_buffer)
        {
            logger::error("CubeRenderPass: Missing required geometry resources");
            return;
        }

        // Check if we have a material or legacy pipeline
        auto material = config_.material_ref.lock();
        if (!material && !config_.pipeline)
        {
            logger::error("CubeRenderPass: Missing pipeline or material");
            return;
        }

        // Begin render pass
        VkClearValue clear_values[2];
        clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clear_values[1].depthStencil = {1.0f, 0};

        VkRect2D render_area{};
        render_area.offset = {0, 0};
        render_area.extent = {ctx.width, ctx.height};

        cmd.begin_render_pass(ctx.render_pass, ctx.framebuffer, render_area, {clear_values[0], clear_values[1]});

        // Bind pipeline (prefer material if available)
        if (material)
        {
            material->bind(cmd);
        }
        else
        {
            cmd.bind_graphics_pipeline(*config_.pipeline);
        }

        // Set viewport
        cmd.set_viewport(
                         0.0f,
                         0.0f,
                         static_cast<float>(config_.width),
                         static_cast<float>(config_.height),
                         0.0f,
                         1.0f
                        );

        // Set scissor
        cmd.set_scissor(0, 0, config_.width, config_.height);

        // Bind descriptor set for MVP (legacy path, material handles its own descriptors)
        if (!material && config_.frame_index < config_.descriptor_sets.size())
        {
            VkDescriptorSet descriptor_set = config_.descriptor_sets[config_.frame_index];
            cmd.bind_descriptor_sets(config_.pipeline_layout, 0, {descriptor_set});
        }

        // Push MVP matrix (only if using material system with push constant support)
        if (material)
        {
            cmd.push_constants(material->pipeline_layout(),
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,
                               sizeof(glm::mat4),
                               &current_mvp_);
        }
        // Legacy pipeline doesn't have push constant range, skip

        // Bind vertex buffer
        cmd.bind_vertex_buffer(config_.vertex_buffer->handle(), 0);

        // Bind index buffer
        cmd.bind_index_buffer(config_.index_buffer->handle(), VK_INDEX_TYPE_UINT16);

        // Draw indexed
        cmd.draw_indexed(config_.index_count, 1, 0, 0, 0);

        // End render pass
        cmd.end_render_pass();
    }

    void CubeRenderPass::set_mvp_matrix(const glm::mat4& mvp)
    {
        current_mvp_ = mvp;
    }
} // namespace vulkan_engine::rendering