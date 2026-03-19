#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace vulkan_engine::vulkan
{
    class PipelineLayout
    {
        public:
            PipelineLayout() = default;
            PipelineLayout(
                std::shared_ptr<DeviceManager>            device,
                const std::vector<VkDescriptorSetLayout>& set_layouts    = {},
                const std::vector<VkPushConstantRange>&   push_constants = {});
            ~PipelineLayout();

            // Non-copyable
            PipelineLayout(const PipelineLayout&)            = delete;
            PipelineLayout& operator=(const PipelineLayout&) = delete;

            // Movable
            PipelineLayout(PipelineLayout&& other) noexcept;
            PipelineLayout& operator=(PipelineLayout&& other) noexcept;

            VkPipelineLayout handle() const { return layout_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkPipelineLayout               layout_ = VK_NULL_HANDLE;
    };

    struct GraphicsPipelineConfig
    {
        // Shaders
        std::string vertex_shader_path;
        std::string fragment_shader_path;
        std::string geometry_shader_path;

        // Vertex input
        std::vector<VkVertexInputBindingDescription>   vertex_bindings;
        std::vector<VkVertexInputAttributeDescription> vertex_attributes;

        // Input assembly
        VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Rasterization
        VkPolygonMode   polygon_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cull_mode    = VK_CULL_MODE_BACK_BIT;
        VkFrontFace     front_face   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // Depth and stencil
        bool        depth_test_enable  = true;
        bool        depth_write_enable = true;
        VkCompareOp depth_compare_op   = VK_COMPARE_OP_LESS;

        // Blending
        bool blend_enable = false;

        // Pipeline layout and render pass
        VkPipelineLayout layout      = VK_NULL_HANDLE;
        VkRenderPass     render_pass = VK_NULL_HANDLE;
        uint32_t         subpass     = 0;

        // Dynamic Rendering formats (used when render_pass is VK_NULL_HANDLE)
        VkFormat color_format = VK_FORMAT_UNDEFINED;
        VkFormat depth_format = VK_FORMAT_UNDEFINED;
    };

    class GraphicsPipeline
    {
        public:
            GraphicsPipeline(std::shared_ptr<DeviceManager> device, const GraphicsPipelineConfig& config);
            ~GraphicsPipeline();

            // Non-copyable
            GraphicsPipeline(const GraphicsPipeline&)            = delete;
            GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

            // Movable
            GraphicsPipeline(GraphicsPipeline&& other) noexcept;
            GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

            void bind(VkCommandBuffer cmd);
            void set_viewport(VkCommandBuffer cmd, float x, float y, float width, float height);
            void set_scissor(VkCommandBuffer cmd, int32_t x, int32_t y, uint32_t width, uint32_t height);

            VkPipeline       handle() const { return pipeline_; }
            VkPipelineLayout layout() const { return layout_; }

        private:
            VkShaderModule load_shader_module(const std::string& path);

            std::shared_ptr<DeviceManager>  device_;
            VkPipeline                      pipeline_ = VK_NULL_HANDLE;
            VkPipelineLayout                layout_   = VK_NULL_HANDLE;
            std::unique_ptr<PipelineLayout> owned_layout_;
    };

    class PipelineCache
    {
        public:
            explicit PipelineCache(std::shared_ptr<DeviceManager> device);
            ~PipelineCache();

            // Non-copyable
            PipelineCache(const PipelineCache&)            = delete;
            PipelineCache& operator=(const PipelineCache&) = delete;

            VkPipelineCache handle() const { return cache_; }

            std::vector<uint8_t> get_data() const;
            void                 merge(const std::vector<VkPipelineCache>& caches);
            void                 save_to_file(const std::string& path);
            void                 load_from_file(const std::string& path);

        private:
            std::shared_ptr<DeviceManager> device_;
            VkPipelineCache                cache_ = VK_NULL_HANDLE;
    };
} // namespace vulkan_engine::vulkan