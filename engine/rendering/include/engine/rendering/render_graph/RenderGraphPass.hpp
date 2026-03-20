#pragma once

#include "engine/rendering/render_graph/RenderGraph.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <array>

// Forward declarations for Vulkan backend types to reduce layer coupling
namespace vulkan_engine::vulkan
{
    class RenderCommandBuffer;
    class GraphicsPipeline;
    class Buffer;
}

namespace vulkan_engine::rendering
{
    // Forward declarations
    class RenderGraphResourcePool;

    // Render context passed to passes during execution
    struct RenderContext
    {
        uint32_t frame_index  = 0;
        uint32_t image_index  = 0;
        uint32_t width        = 0;
        uint32_t height       = 0;
        float    delta_time   = 0.0f;
        float    elapsed_time = 0.0f;

        // Vulkan objects (for traditional render pass)
        VkRenderPass  render_pass = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;

        // Dynamic Rendering attachments (alternative to render_pass/framebuffer)
        VkImageView color_image_view = VK_NULL_HANDLE;
        VkImageView depth_image_view = VK_NULL_HANDLE;

        // Device
        std::shared_ptr<vulkan::DeviceManager> device;
    };

    // Resource barriers for automatic synchronization
    struct ResourceBarrier
    {
        ImageHandle          image;
        VkImageLayout        old_layout;
        VkImageLayout        new_layout;
        VkAccessFlags        src_access;
        VkAccessFlags        dst_access;
        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;
    };

    // Base class for all render passes with resource tracking
    class RenderPassBase : public RenderGraphNode
    {
        public:
            virtual ~RenderPassBase() = default;

            // Execute the pass with full context
            virtual void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) = 0;

            // Setup phase - declare resource usage
            virtual void setup(RenderGraphBuilder& builder) = 0;

            // Get required barriers for this pass
            virtual std::vector<ResourceBarrier> get_barriers() const { return {}; }

            // RenderGraphNode interface implementation
            void             execute(CommandBuffer& cmd) override;
            std::string_view name() const override { return name_; }

            std::vector<BufferHandle> get_buffer_inputs() const override { return buffer_inputs_; }
            std::vector<ImageHandle>  get_image_inputs() const override { return image_inputs_; }
            std::vector<BufferHandle> get_buffer_outputs() const override { return buffer_outputs_; }
            std::vector<ImageHandle>  get_image_outputs() const override { return image_outputs_; }

        protected:
            std::string               name_;
            std::vector<BufferHandle> buffer_inputs_;
            std::vector<ImageHandle>  image_inputs_;
            std::vector<BufferHandle> buffer_outputs_;
            std::vector<ImageHandle>  image_outputs_;
    };

    // Clear render pass - clears attachments
    class ClearRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string          name               = "ClearPass";
                std::array<float, 4> color              = {0.0f, 0.0f, 0.0f, 1.0f};
                float                depth              = 1.0f;
                uint32_t             stencil            = 0;
                bool                 enable_color_clear = true;
                bool                 enable_depth_clear = true;
                ImageHandle          color_output; // Output color attachment
                ImageHandle          depth_output; // Output depth attachment
            };

            explicit ClearRenderPass(const Config& config);

            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

        private:
            Config config_;
    };

    // Geometry render pass - renders meshes
    class GeometryRenderPass : public RenderPassBase
    {
        public:
            struct MeshDraw
            {
                vulkan::Buffer* vertex_buffer = nullptr;
                vulkan::Buffer* index_buffer  = nullptr;
                uint32_t        index_count   = 0;
                VkIndexType     index_type    = VK_INDEX_TYPE_UINT16;
                uint32_t        vertex_offset = 0;

                // Transform
                glm::mat4 model_matrix = glm::mat4(1.0f);

                // Pipeline and descriptors
                vulkan::GraphicsPipeline* pipeline        = nullptr;
                VkPipelineLayout          pipeline_layout = VK_NULL_HANDLE;
                VkDescriptorSet           descriptor_set  = VK_NULL_HANDLE;
            };

            struct Config
            {
                std::string           name = "GeometryPass";
                std::vector<MeshDraw> meshes;

                // Viewport
                float viewport_x      = 0.0f;
                float viewport_y      = 0.0f;
                float viewport_width  = 1.0f; // 1.0 = full width
                float viewport_height = 1.0f; // 1.0 = full height

                // Depth/stencil
                bool enable_depth_test  = true;
                bool enable_depth_write = true;
            };

            explicit GeometryRenderPass(const Config& config);

            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

            // Add mesh to render
            void add_mesh(const MeshDraw& mesh);
            void clear_meshes();

            // Update config
            void    set_config(const Config& config) { config_ = config; }
            Config& config() { return config_; }

        private:
            Config config_;
    };

    // Present render pass - transitions image for presentation
    class PresentRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "PresentPass";
                ImageHandle source_image; // Image to present (if not using swap chain directly)
            };

            explicit PresentRenderPass(const Config& config = {});

            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

        private:
            Config config_;
    };

    // UI/ImGui render pass (placeholder for future)
    class UIRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "UIPass";
            };

            explicit UIRenderPass(const Config& config = {});

            void setup(RenderGraphBuilder& builder) override;
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;

        private:
            Config config_;
    };
} // namespace vulkan_engine::rendering