#pragma once

#include "Core.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include <vector>
#include <optional>

namespace engine::rhi
{
    // Forward declarations
    class Shader;
    using ShaderHandle = std::shared_ptr<Shader>;

    // Shader description
    struct ShaderDesc
    {
        ShaderStage           stage = ShaderStage::None;
        std::vector<uint32_t> spirvCode; // SPIR-V binary
        std::string           entryPoint = "main";
    };

    // Shader class
    class Shader
    {
        public:
            Shader() = default;
            ~Shader();

            Shader(const Shader&)            = delete;
            Shader& operator=(const Shader&) = delete;

            Shader(Shader&& other) noexcept;
            Shader& operator=(Shader&& other) noexcept;

            [[nodiscard]] bool               isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkShaderModule     nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] ShaderStage        stage() const noexcept { return stage_; }
            [[nodiscard]] const std::string& entryPoint() const noexcept { return entryPoint_; }

            struct InternalData
            {
                VkShaderModule shader = nullptr;
                VkDevice       device = nullptr;
            };

            explicit Shader(const InternalData& data, ShaderStage stage, std::string entryPoint);
            void     release();

        private:
            VkShaderModule handle_ = nullptr;
            VkDevice       device_ = nullptr;
            ShaderStage    stage_;
            std::string    entryPoint_;
    };

    // Vertex input binding description
    struct VertexInputBindingDesc
    {
        uint32_t        binding   = 0;
        uint32_t        stride    = 0;
        VertexInputRate inputRate = VertexInputRate::Vertex;
    };

    // Vertex attribute description
    struct VertexAttributeDesc
    {
        uint32_t location = 0;
        uint32_t binding  = 0;
        Format   format   = Format::Undefined;
        uint32_t offset   = 0;
    };

    // Vertex input state
    struct VertexInputState
    {
        std::vector<VertexInputBindingDesc> bindings;
        std::vector<VertexAttributeDesc>    attributes;
    };

    // Input assembly state
    struct InputAssemblyState
    {
        PrimitiveTopology topology               = PrimitiveTopology::TriangleList;
        bool              primitiveRestartEnable = false;
    };

    // Rasterization state
    struct RasterizationState
    {
        bool        depthClampEnable        = false;
        bool        rasterizerDiscardEnable = false;
        PolygonMode polygonMode             = PolygonMode::Fill;
        CullMode    cullMode                = CullMode::Back;
        FrontFace   frontFace               = FrontFace::CounterClockwise;
        bool        depthBiasEnable         = false;
        float       depthBiasConstantFactor = 0.0f;
        float       depthBiasClamp          = 0.0f;
        float       depthBiasSlopeFactor    = 0.0f;
        float       lineWidth               = 1.0f;
    };

    // Multisample state
    struct MultisampleState
    {
        SampleCount rasterizationSamples = SampleCount::Samples1;
        bool        sampleShadingEnable  = false;
        float       minSampleShading     = 1.0f;
        // Sample mask can be added if needed
        bool alphaToCoverageEnable = false;
        bool alphaToOneEnable      = false;
    };

    // Depth stencil state
    struct DepthStencilState
    {
        bool      depthTestEnable       = true;
        bool      depthWriteEnable      = true;
        CompareOp depthCompareOp        = CompareOp::Less;
        bool      depthBoundsTestEnable = false;
        bool      stencilTestEnable     = false;
        // Stencil ops can be added if needed
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
    };

    // Color component flags
    enum class ColorComponentFlags : uint32_t
    {
        None = 0,
        R    = 1 << 0,
        G    = 1 << 1,
        B    = 1 << 2,
        A    = 1 << 3,
        All  = R | G | B | A,
    };

    // Blend attachment state
    struct BlendAttachmentState
    {
        bool                blendEnable         = false;
        BlendFactor         srcColorBlendFactor = BlendFactor::One;
        BlendFactor         dstColorBlendFactor = BlendFactor::Zero;
        BlendOp             colorBlendOp        = BlendOp::Add;
        BlendFactor         srcAlphaBlendFactor = BlendFactor::One;
        BlendFactor         dstAlphaBlendFactor = BlendFactor::Zero;
        BlendOp             alphaBlendOp        = BlendOp::Add;
        ColorComponentFlags colorWriteMask      = ColorComponentFlags::All;
    };


    inline ColorComponentFlags operator|(ColorComponentFlags a, ColorComponentFlags b)
    {
        return static_cast<ColorComponentFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool hasFlag(ColorComponentFlags flags, ColorComponentFlags flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    // Color blend state
    struct ColorBlendState
    {
        bool logicOpEnable = false;
        // LogicOp logicOp;  // Can be added if needed
        std::vector<BlendAttachmentState> attachments;
        float                             blendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    // Viewport state (dynamic by default)
    struct ViewportState
    {
        // Viewports and scissors are typically dynamic
        uint32_t viewportCount = 1;
        uint32_t scissorCount  = 1;
    };

    // Dynamic state flags
    enum class DynamicState : uint32_t
    {
        None               = 0,
        Viewport           = 1 << 0,
        Scissor            = 1 << 1,
        LineWidth          = 1 << 2,
        DepthBias          = 1 << 3,
        BlendConstants     = 1 << 4,
        DepthBounds        = 1 << 5,
        StencilCompareMask = 1 << 6,
        StencilWriteMask   = 1 << 7,
        StencilReference   = 1 << 8,
    };

    inline DynamicState operator|(DynamicState a, DynamicState b)
    {
        return static_cast<DynamicState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    // Descriptor binding type
    enum class DescriptorType
    {
        Sampler,
        CombinedImageSampler,
        SampledImage,
        StorageImage,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformBuffer,
        StorageBuffer,
        UniformBufferDynamic,
        StorageBufferDynamic,
        InputAttachment,
    };

    // Descriptor set layout binding
    struct DescriptorSetLayoutBinding
    {
        uint32_t       binding         = 0;
        DescriptorType descriptorType  = DescriptorType::UniformBuffer;
        uint32_t       descriptorCount = 1;
        ShaderStage    stageFlags      = ShaderStage::None;
        // Immutable samplers can be added if needed
    };

    // Descriptor set layout description
    struct DescriptorSetLayoutDesc
    {
        std::vector<DescriptorSetLayoutBinding> bindings;
    };

    // Descriptor set layout class
    class DescriptorSetLayout
    {
        public:
            DescriptorSetLayout() = default;
            ~DescriptorSetLayout();

            DescriptorSetLayout(const DescriptorSetLayout&)            = delete;
            DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

            DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
            DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

            [[nodiscard]] bool                           isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkDescriptorSetLayout          nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] const DescriptorSetLayoutDesc& desc() const noexcept { return desc_; }

            struct InternalData
            {
                VkDescriptorSetLayout layout = nullptr;
                VkDevice              device = nullptr;
            };

            explicit DescriptorSetLayout(const InternalData& data, const DescriptorSetLayoutDesc& desc);
            void     release();

        private:
            VkDescriptorSetLayout   handle_ = nullptr;
            VkDevice                device_ = nullptr;
            DescriptorSetLayoutDesc desc_;
    };

    using DescriptorSetLayoutHandle = std::shared_ptr<DescriptorSetLayout>;

    // Push constant range
    struct PushConstantRange
    {
        ShaderStage stageFlags = ShaderStage::None;
        uint32_t    offset     = 0;
        uint32_t    size       = 0;
    };

    // Pipeline layout description
    struct PipelineLayoutDesc
    {
        std::vector<DescriptorSetLayoutHandle> setLayouts;
        std::vector<PushConstantRange>         pushConstantRanges;
    };

    // Pipeline layout class
    class PipelineLayout
    {
        public:
            PipelineLayout() = default;
            ~PipelineLayout();

            PipelineLayout(const PipelineLayout&)            = delete;
            PipelineLayout& operator=(const PipelineLayout&) = delete;

            PipelineLayout(PipelineLayout&& other) noexcept;
            PipelineLayout& operator=(PipelineLayout&& other) noexcept;

            [[nodiscard]] bool                      isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkPipelineLayout          nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] const PipelineLayoutDesc& desc() const noexcept { return desc_; }

            struct InternalData
            {
                VkPipelineLayout layout = nullptr;
                VkDevice         device = nullptr;
            };

            explicit PipelineLayout(const InternalData& data, const PipelineLayoutDesc& desc);
            void     release();

        private:
            VkPipelineLayout   handle_ = nullptr;
            VkDevice           device_ = nullptr;
            PipelineLayoutDesc desc_;
    };

    using PipelineLayoutHandle = std::shared_ptr<PipelineLayout>;

    // Graphics pipeline description
    struct GraphicsPipelineDesc
    {
        ShaderHandle                vertexShader;
        ShaderHandle                fragmentShader;
        std::optional<ShaderHandle> geometryShader;
        std::optional<ShaderHandle> tessControlShader;
        std::optional<ShaderHandle> tessEvaluationShader;

        VertexInputState   vertexInputState;
        InputAssemblyState inputAssemblyState;
        RasterizationState rasterizationState;
        MultisampleState   multisampleState;
        DepthStencilState  depthStencilState;
        ColorBlendState    colorBlendState;
        ViewportState      viewportState;
        DynamicState       dynamicStates = DynamicState::Viewport | DynamicState::Scissor;

        PipelineLayoutHandle layout;
        // Render pass and subpass can be added if using traditional render passes
        // For dynamic rendering, these may be null
    };

    // Graphics pipeline class
    class GraphicsPipeline
    {
        public:
            GraphicsPipeline() = default;
            ~GraphicsPipeline();

            GraphicsPipeline(const GraphicsPipeline&)            = delete;
            GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

            GraphicsPipeline(GraphicsPipeline&& other) noexcept;
            GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

            [[nodiscard]] bool                 isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkPipeline           nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] PipelineLayoutHandle layout() const { return layout_; }

            struct InternalData
            {
                VkPipeline           pipeline = nullptr;
                VkDevice             device   = nullptr;
                PipelineLayoutHandle layout;
            };

            explicit GraphicsPipeline(const InternalData& data);
            void     release();

        private:
            VkPipeline           handle_ = nullptr;
            VkDevice             device_ = nullptr;
            PipelineLayoutHandle layout_;
    };

    using GraphicsPipelineHandle = std::shared_ptr<GraphicsPipeline>;

    // Compute pipeline description
    struct ComputePipelineDesc
    {
        ShaderHandle         computeShader;
        PipelineLayoutHandle layout;
    };

    // Compute pipeline class
    class ComputePipeline
    {
        public:
            ComputePipeline() = default;
            ~ComputePipeline();

            ComputePipeline(const ComputePipeline&)            = delete;
            ComputePipeline& operator=(const ComputePipeline&) = delete;

            ComputePipeline(ComputePipeline&& other) noexcept;
            ComputePipeline& operator=(ComputePipeline&& other) noexcept;

            [[nodiscard]] bool                 isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkPipeline           nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] PipelineLayoutHandle layout() const { return layout_; }

            struct InternalData
            {
                VkPipeline           pipeline = nullptr;
                VkDevice             device   = nullptr;
                PipelineLayoutHandle layout;
            };

            explicit ComputePipeline(const InternalData& data);
            void     release();

        private:
            VkPipeline           handle_ = nullptr;
            VkDevice             device_ = nullptr;
            PipelineLayoutHandle layout_;
    };

    using ComputePipelineHandle = std::shared_ptr<ComputePipeline>;
} // namespace engine::rhi
