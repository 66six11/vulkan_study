#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/pipelines/ShaderModule.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <fstream>
#include <stdexcept>

namespace vulkan_engine::vulkan
{
    // PipelineLayout implementation
    PipelineLayout::PipelineLayout(
        std::shared_ptr<DeviceManager>            device,
        const std::vector<VkDescriptorSetLayout>& set_layouts,
        const std::vector<VkPushConstantRange>&   push_constants)
        : device_(std::move(device))
    {
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount         = static_cast<uint32_t>(set_layouts.size());
        layout_info.pSetLayouts            = set_layouts.data();
        layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
        layout_info.pPushConstantRanges    = push_constants.data();

        VkResult result = vkCreatePipelineLayout(device_->device(), &layout_info, nullptr, &layout_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create pipeline layout", __FILE__, __LINE__);
        }
    }

    PipelineLayout::~PipelineLayout()
    {
        if (layout_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyPipelineLayout(device_->device(), layout_, nullptr);
        }
    }

    PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
        : device_(std::move(other.device_))
        , layout_(other.layout_)
    {
        other.layout_ = VK_NULL_HANDLE;
    }

    PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept
    {
        if (this != &other)
        {
            if (layout_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyPipelineLayout(device_->device(), layout_, nullptr);
            }

            device_       = std::move(other.device_);
            layout_       = other.layout_;
            other.layout_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    // GraphicsPipeline implementation
    GraphicsPipeline::GraphicsPipeline(std::shared_ptr<DeviceManager> device, const GraphicsPipelineConfig& config)
        : device_(std::move(device))
    {
        // Load shaders using ShaderModule (RAII)
        std::vector<ShaderModule>                    shader_modules;
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

        // Vertex shader
        if (!config.vertex_shader_path.empty())
        {
            shader_modules.emplace_back(device_, config.vertex_shader_path);

            VkPipelineShaderStageCreateInfo vert_stage{};
            vert_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_stage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
            vert_stage.module = shader_modules.back().handle();
            vert_stage.pName  = "main";
            shader_stages.push_back(vert_stage);
        }

        // Fragment shader
        if (!config.fragment_shader_path.empty())
        {
            shader_modules.emplace_back(device_, config.fragment_shader_path);

            VkPipelineShaderStageCreateInfo frag_stage{};
            frag_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_stage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_stage.module = shader_modules.back().handle();
            frag_stage.pName  = "main";
            shader_stages.push_back(frag_stage);
        }

        // Geometry shader (optional)
        if (!config.geometry_shader_path.empty())
        {
            shader_modules.emplace_back(device_, config.geometry_shader_path);

            VkPipelineShaderStageCreateInfo geom_stage{};
            geom_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            geom_stage.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
            geom_stage.module = shader_modules.back().handle();
            geom_stage.pName  = "main";
            shader_stages.push_back(geom_stage);
        }

        // Vertex input
        VkPipelineVertexInputStateCreateInfo vertex_input{};
        vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount   = static_cast<uint32_t>(config.vertex_bindings.size());
        vertex_input.pVertexBindingDescriptions      = config.vertex_bindings.data();
        vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertex_attributes.size());
        vertex_input.pVertexAttributeDescriptions    = config.vertex_attributes.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology               = config.primitive_topology;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and scissor (dynamic)
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount  = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = config.polygon_mode;
        rasterizer.lineWidth               = 1.0f;
        rasterizer.cullMode                = config.cull_mode;
        rasterizer.frontFace               = config.front_face;
        rasterizer.depthBiasEnable         = VK_FALSE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable  = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable       = config.depth_test_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthWriteEnable      = config.depth_write_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthCompareOp        = config.depth_compare_op;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable     = VK_FALSE;

        // Color blending
        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable         = config.blend_enable ? VK_TRUE : VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable   = VK_FALSE;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments    = &color_blend_attachment;

        // Dynamic states
        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates    = dynamic_states.data();

        // Create pipeline layout if not provided
        VkPipelineLayout layout = config.layout;
        if (layout == VK_NULL_HANDLE)
        {
            owned_layout_ = std::make_unique < PipelineLayout > (device_);
            layout        = owned_layout_->handle();
        }
        else
        {
            owned_layout_ = nullptr;
        }

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages             = shader_stages.data();
        pipeline_info.pVertexInputState   = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState      = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState   = &multisampling;
        pipeline_info.pDepthStencilState  = &depth_stencil;
        pipeline_info.pColorBlendState    = &color_blending;
        pipeline_info.pDynamicState       = &dynamic_state;
        pipeline_info.layout              = layout;
        pipeline_info.renderPass          = config.render_pass;
        pipeline_info.subpass             = config.subpass;
        pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

        VkResult result = vkCreateGraphicsPipelines(device_->device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create graphics pipeline", __FILE__, __LINE__);
        }

        // Shader modules are automatically cleaned up by ShaderModule RAII
        // when shader_modules vector goes out of scope
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        if (pipeline_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyPipeline(device_->device(), pipeline_, nullptr);
        }
    }

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
        : device_(std::move(other.device_))
        , pipeline_(other.pipeline_)
        , layout_(other.layout_)
        , owned_layout_(std::move(other.owned_layout_))
    {
        other.pipeline_ = VK_NULL_HANDLE;
        other.layout_   = VK_NULL_HANDLE;
    }

    GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept
    {
        if (this != &other)
        {
            if (pipeline_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyPipeline(device_->device(), pipeline_, nullptr);
            }

            device_       = std::move(other.device_);
            pipeline_     = other.pipeline_;
            layout_       = other.layout_;
            owned_layout_ = std::move(other.owned_layout_);

            other.pipeline_ = VK_NULL_HANDLE;
            other.layout_   = VK_NULL_HANDLE;
        }
        return *this;
    }

    void GraphicsPipeline::bind(VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    }

    void GraphicsPipeline::set_viewport(VkCommandBuffer cmd, float x, float y, float width, float height)
    {
        VkViewport viewport{};
        viewport.x        = x;
        viewport.y        = y;
        viewport.width    = width;
        viewport.height   = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
    }

    void GraphicsPipeline::set_scissor(VkCommandBuffer cmd, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        VkRect2D scissor{};
        scissor.offset.x      = x;
        scissor.offset.y      = y;
        scissor.extent.width  = width;
        scissor.extent.height = height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    // PipelineCache implementation
    PipelineCache::PipelineCache(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
        VkPipelineCacheCreateInfo cache_info{};
        cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkResult result = vkCreatePipelineCache(device_->device(), &cache_info, nullptr, &cache_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create pipeline cache", __FILE__, __LINE__);
        }
    }

    PipelineCache::~PipelineCache()
    {
        if (cache_ != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(device_->device(), cache_, nullptr);
        }
    }

    std::vector<uint8_t> PipelineCache::get_data() const
    {
        size_t data_size = 0;
        vkGetPipelineCacheData(device_->device(), cache_, &data_size, nullptr);

        std::vector<uint8_t> data(data_size);
        vkGetPipelineCacheData(device_->device(), cache_, &data_size, data.data());

        return data;
    }

    void PipelineCache::merge(const std::vector<VkPipelineCache>& caches)
    {
        vkMergePipelineCaches(device_->device(), cache_, static_cast<uint32_t>(caches.size()), caches.data());
    }

    void PipelineCache::save_to_file(const std::string& path)
    {
        auto data = get_data();

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open pipeline cache file for writing: " + path);
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
    }

    void PipelineCache::load_from_file(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            // File doesn't exist, that's okay - cache will be empty
            return;
        }

        auto                 file_size = static_cast<size_t>(file.tellg());
        std::vector<uint8_t> data(file_size);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), file_size);
        file.close();

        // Destroy old cache
        if (cache_ != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(device_->device(), cache_, nullptr);
        }

        // Create new cache with initial data
        VkPipelineCacheCreateInfo cache_info{};
        cache_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cache_info.initialDataSize = data.size();
        cache_info.pInitialData    = data.data();

        VkResult result = vkCreatePipelineCache(device_->device(), &cache_info, nullptr, &cache_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create pipeline cache from file", __FILE__, __LINE__);
        }
    }
} // namespace vulkan_engine::vulkan
