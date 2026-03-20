#include "engine/rendering/render_graph/CubeRenderPass.hpp"
#include "engine/core/utils/Logger.hpp"
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

        auto material = config_.material_ref.lock();
        if (!material)
        {
            logger::error("CubeRenderPass: Material not available");
            return;
        }

        // Bind material
        material->bind(cmd);

        // Set viewport
        cmd.set_viewport(
                         0.0f,
                         0.0f,
                         static_cast<float>(ctx.width),
                         static_cast<float>(ctx.height),
                         0.0f,
                         1.0f);

        // Set scissor
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
    }

    void CubeRenderPass::set_mvp_matrix(const glm::mat4& mvp)
    {
        current_mvp_ = mvp;
    }
} // namespace vulkan_engine::rendering