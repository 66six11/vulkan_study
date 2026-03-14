#pragma once

#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/material/Material.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/resources/UniformBuffer.hpp"

#include <memory>
#include <glm/glm.hpp>

namespace vulkan_engine::rendering
{
    // Uniform buffer object for MVP matrix
    struct CubeUniformBufferObject
    {
        glm::mat4 mvp;
    };

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

                // Material (optional, replaces pipeline/layout/descriptor_sets)
                // Using weak_ptr to avoid dangling pointer if Material is destroyed
                std::weak_ptr<Material> material_ref;

                // Legacy: Pipeline and layout (used if material is null)
                vulkan::GraphicsPipeline* pipeline        = nullptr;
                VkPipelineLayout          pipeline_layout = VK_NULL_HANDLE;

                // Legacy: Descriptor sets for MVP (per frame)
                std::vector<VkDescriptorSet> descriptor_sets;

                // Resources
                ImageHandle color_output; // Swap chain image
                ImageHandle depth_output; // Depth buffer

                // Viewport
                uint32_t width  = 1280;
                uint32_t height = 720;

                // Frame index for double buffering
                uint32_t frame_index = 0;
            };

            explicit CubeRenderPass(const Config& config);
            ~CubeRenderPass() override = default;

            // RenderPassBase interface
            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

            // Update config (for frame index changes)
            void set_frame_index(uint32_t frame_index) { config_.frame_index = frame_index; }
            void set_mvp_matrix(const glm::mat4& mvp);
            void set_material(std::shared_ptr<Material> material) { config_.material_ref = material; }

        private:
            Config    config_;
            glm::mat4 current_mvp_ = glm::mat4(1.0f);
    };
} // namespace vulkan_engine::rendering