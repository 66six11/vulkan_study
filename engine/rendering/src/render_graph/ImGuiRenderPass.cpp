#include "engine/rendering/render_graph/ImGuiRenderPass.hpp"
#include "engine/core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    ImGuiRenderPass::ImGuiRenderPass(const Config& config)
        : config_(config)
    {
        name_ = config.name;
    }

    void ImGuiRenderPass::setup(RenderGraphBuilder& /*builder*/)
    {
        // ImGui renders to the swap chain image, no additional resources needed
    }

    void ImGuiRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        // Set viewport and scissor for ImGui
        cmd.set_viewport(
                         0.0f,
                         0.0f,
                         static_cast<float>(ctx.width),
                         static_cast<float>(ctx.height),
                         0.0f,
                         1.0f);

        cmd.set_scissor(0, 0, ctx.width, ctx.height);

        // Render ImGui draw data
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data)
        {
            ImGui_ImplVulkan_RenderDrawData(draw_data, cmd.handle());
        }
    }
} // namespace vulkan_engine::rendering
