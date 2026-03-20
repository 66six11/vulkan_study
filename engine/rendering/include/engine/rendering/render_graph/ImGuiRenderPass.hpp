#pragma once

#include "engine/rendering/render_graph/RenderGraphPass.hpp"
#include "engine/rhi/vulkan/command/CommandBuffer.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // ImGuiRenderPass - Renders ImGui UI using Dynamic Rendering
    // ============================================================================
    class ImGuiRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name   = "ImGuiRenderPass";
                uint32_t    width  = 0;
                uint32_t    height = 0;
            };

            explicit ImGuiRenderPass(const Config& config);
            ~ImGuiRenderPass() override = default;

            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

            // Update viewport size
            void set_viewport_size(uint32_t width, uint32_t height)
            {
                config_.width  = width;
                config_.height = height;
            }

        private:
            Config config_;
    };
} // namespace vulkan_engine::rendering