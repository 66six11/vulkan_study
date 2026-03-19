#pragma once

#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/Image.hpp"
#include "vulkan/resources/UniformBuffer.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/command/CommandBuffer.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <mutex>

#include <glm/glm.hpp>

namespace vulkan_engine::rendering
{
    // Material parameter types
    using MaterialParamValue = std::variant<
        float,      // Scalar
        glm::vec2,  // Vector2
        glm::vec3,  // Vector3
        glm::vec4,  // Vector4
        int,        // Integer
        bool,       // Boolean
        std::string // Texture path
    >;

    // Material parameter descriptor
    struct MaterialParam
    {
        std::string        name;
        MaterialParamValue value;
        uint32_t           binding; // Shader binding point
        uint32_t           offset;  // Offset in uniform buffer
    };

    // Material definition (loaded from JSON)
    struct MaterialDefinition
    {
        std::string                                    name;
        std::string                                    shader_name; // References a shader program
        std::unordered_map<std::string, MaterialParam> parameters;
        std::vector<std::string>                       texture_bindings; // Texture slot names
    };

    // ============================================================================
    // Material - Encapsulates shader, parameters, and textures for rendering
    // ============================================================================
    class Material
    {
        public:
            struct Config
            {
                std::string     name = "DefaultMaterial";
                std::string     vertex_shader_path;
                std::string     fragment_shader_path;
                VkRenderPass    render_pass      = VK_NULL_HANDLE;
                bool            depth_test       = true;
                bool            depth_write      = true;
                VkCompareOp     depth_compare_op = VK_COMPARE_OP_LESS;
                bool            blend_enable     = false;
                VkCullModeFlags cull_mode        = VK_CULL_MODE_BACK_BIT; // 默认启用背面剔除

                // Dynamic Rendering formats (used when render_pass is VK_NULL_HANDLE)
                VkFormat color_format = VK_FORMAT_UNDEFINED;
                VkFormat depth_format = VK_FORMAT_UNDEFINED;
            };

            Material(std::shared_ptr<vulkan::DeviceManager> device, const Config& config);
            ~Material();

            // Non-copyable
            Material(const Material&)            = delete;
            Material& operator=(const Material&) = delete;

            // Movable
            Material(Material&& other) noexcept;
            Material& operator=(Material&& other) noexcept;

            // Build the pipeline for dynamic rendering
            void build(VkFormat color_format, VkFormat depth_format);

            // Bind material for rendering
            void bind(vulkan::RenderCommandBuffer& cmd);

            // Set parameter values
            void set_float(const std::string& name, float value);
            void set_vec2(const std::string& name, const glm::vec2& value);
            void set_vec3(const std::string& name, const glm::vec3& value);
            void set_vec4(const std::string& name, const glm::vec4& value);
            void set_int(const std::string& name, int value);
            void set_bool(const std::string& name, bool value);
            void set_texture(const std::string& name, std::shared_ptr<vulkan::Image> texture, VkImageView view);

            // Getters
            const std::string& name() const { return config_.name; }
            bool               is_built() const { return pipeline_ != nullptr; }
            VkPipelineLayout   pipeline_layout() const { return pipeline_layout_; }

            // Access to pipeline
            vulkan::GraphicsPipeline* pipeline() const { return pipeline_.get(); }

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;
            Config                                 config_;

            // Pipeline for dynamic rendering
            std::unique_ptr<vulkan::GraphicsPipeline> pipeline_;
            VkPipelineLayout                          pipeline_layout_       = VK_NULL_HANDLE;
            VkDescriptorSetLayout                     descriptor_set_layout_ = VK_NULL_HANDLE;

            // Uniform buffer for material parameters (std140 layout)
            struct UniformBufferData
            {
                glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // offset 0,   size 16
                float     roughness   = 0.5f;            // offset 16,  size 4
                float     metallic    = 0.0f;            // offset 20,  size 4
                float     emissive    = 0.0f;            // offset 24,  size 4
                float     has_texture = 0.0f;            // offset 28,  size 4
                // New fields for vec2/int/bool support
                glm::vec2 uv_scale{1.0f, 1.0f};  // offset 32,  size 8
                int       texture_id     = 0;    // offset 40,  size 4
                int       use_normal_map = 0;    // offset 44,  size 4 (bool as int)
                float     padding        = 0.0f; // offset 48,  size 4 (padding to 64 bytes)
            };

            std::unique_ptr<vulkan::UniformBuffer<UniformBufferData>> uniform_buffer_;

            // Descriptor set
            VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
            VkDescriptorSet  descriptor_set_  = VK_NULL_HANDLE;

            // Textures
            struct TextureBinding
            {
                std::shared_ptr<vulkan::Image> image;
                VkImageView                    view = VK_NULL_HANDLE;
                uint32_t                       binding;
            };

            std::unordered_map<std::string, TextureBinding> textures_;

            // Default sampler for texture binding
            VkSampler default_sampler_ = VK_NULL_HANDLE;

            // Default white texture (1x1 pixel, used when no texture is set)
            std::shared_ptr<vulkan::Image> default_white_texture_;
            VkImageView                    default_white_texture_view_ = VK_NULL_HANDLE;

            // Mutex for thread-safe uniform buffer updates
            mutable std::mutex uniform_mutex_;

            // Helper methods
            void create_descriptor_set_layout();
            void create_pipeline_layout();
            void create_descriptor_set();
            void create_default_sampler();
            void create_default_white_texture();
            void update_descriptor_set();
            void update_uniform_buffer();
            void build_internal(VkFormat color_format, VkFormat depth_format);

            void cleanup();
    };
} // namespace vulkan_engine::rendering