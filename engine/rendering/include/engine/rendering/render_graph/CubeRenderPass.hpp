#pragma once

#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/material/Material.hpp"
#include "vulkan/resources/Buffer.hpp"

#include <memory>
#include <glm/glm.hpp>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Cube Render Pass - Renders a rotating cube using Render Graph
    // ============================================================================
    class CubeRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "CubeRenderPass";

                // Geometry data
                vulkan::Buffer* vertex_buffer = nullptr;
                vulkan::Buffer* index_buffer  = nullptr;
                uint32_t        index_count   = 0;
                VkIndexType     index_type    = VK_INDEX_TYPE_UINT16; // Default to 16-bit for cube

                // Material (required for rendering)
                // Using weak_ptr to avoid dangling pointer if Material is destroyed
                std::weak_ptr<Material> material_ref;

                // Resources
                ImageHandle color_output; // Swap chain image
                ImageHandle depth_output; // Depth buffer

                // Viewport
                uint32_t width  = 1280;
                uint32_t height = 720;
            };

            explicit CubeRenderPass(const Config& config);
            ~CubeRenderPass() override = default;

            // RenderPassBase interface
            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

            void set_mvp_matrix(const glm::mat4& mvp);
            void set_material(std::shared_ptr<Material> material) { config_.material_ref = material; }

        private:
            Config    config_;
            glm::mat4 current_mvp_ = glm::mat4(1.0f);
    };
} // namespace vulkan_engine::rendering