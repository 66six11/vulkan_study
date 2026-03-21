#pragma once

#include <cstdint>
#include <expected>
#include <variant>

// Vulkan forward declarations (dispatchable handles only)
// Non-dispatchable KHR handles require vulkan_core.h due to platform-dependent definitions
struct VkBuffer_T;
struct VkImage_T;
struct VkImageView_T;
struct VkCommandBuffer_T;
struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkInstance_T;
struct VkQueue_T;
struct VkCommandPool_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;
struct VkDescriptorSet_T;
struct VkDescriptorSetLayout_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VkSampler_T;
struct VkRenderPass_T;
struct VkFramebuffer_T;
struct VkShaderModule_T;
struct VmaAllocator_T;
struct VmaAllocation_T;

using VkBuffer              = VkBuffer_T*;
using VkImage               = VkImage_T*;
using VkImageView           = VkImageView_T*;
using VkCommandBuffer       = VkCommandBuffer_T*;
using VkDevice              = VkDevice_T*;
using VkPhysicalDevice      = VkPhysicalDevice_T*;
using VkInstance            = VkInstance_T*;
using VkQueue               = VkQueue_T*;
using VkCommandPool         = VkCommandPool_T*;
using VkPipeline            = VkPipeline_T*;
using VkPipelineLayout      = VkPipelineLayout_T*;
using VkDescriptorSet       = VkDescriptorSet_T*;
using VkDescriptorSetLayout = VkDescriptorSetLayout_T*;
using VkSemaphore           = VkSemaphore_T*;
using VkFence               = VkFence_T*;
using VkSampler             = VkSampler_T*;
using VkRenderPass          = VkRenderPass_T*;
using VkFramebuffer         = VkFramebuffer_T*;
using VkShaderModule        = VkShaderModule_T*;
using VmaAllocator          = VmaAllocator_T*;
using VmaAllocation         = VmaAllocation_T*;

namespace engine::rhi
{
    // Backend selection
    #define RHI_BACKEND_VULKAN 1

    // Result type
    enum class Result
    {
        Success,
        Error_OutOfMemory,
        Error_DeviceLost,
        Error_InvalidParameter,
        Error_Unsupported,
        Error_NotReady,
        Error_Timeout,
        Error_OutOfDate,
        Error_SurfaceLost,
    };

    template <typename T> using ResultValue = std::expected<T, Result>;

    // Basic data structures
    struct Extent2D
    {
        uint32_t width  = 0;
        uint32_t height = 0;
    };

    struct Extent3D
    {
        uint32_t width  = 0;
        uint32_t height = 0;
        uint32_t depth  = 1;
    };

    struct Offset2D
    {
        int32_t x = 0;
        int32_t y = 0;
    };

    struct Offset3D
    {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;
    };

    struct Rect2D
    {
        Offset2D offset;
        Extent2D extent;
    };

    struct Viewport
    {
        float x        = 0.0f;
        float y        = 0.0f;
        float width    = 0.0f;
        float height   = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    // Format enum
    enum class Format
    {
        Undefined,
        R8_UNORM,
        R8G8_UNORM,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R32_FLOAT,
        R32G32_FLOAT,
        R32G32B32_FLOAT,
        R32G32B32A32_FLOAT,
        R32_UINT,
        R32G32_UINT,
        R32G32B32_UINT,
        R32G32B32A32_UINT,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        D16_UNORM,
    };

    // Buffer usage flags
    enum class BufferUsage : uint32_t
    {
        None           = 0,
        TransferSrc    = 1 << 0,
        TransferDst    = 1 << 1,
        UniformBuffer  = 1 << 2,
        StorageBuffer  = 1 << 3,
        IndexBuffer    = 1 << 4,
        VertexBuffer   = 1 << 5,
        IndirectBuffer = 1 << 6,
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b)
    {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline BufferUsage operator&(BufferUsage a, BufferUsage b)
    {
        return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool hasFlag(BufferUsage flags, BufferUsage flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    // Texture usage flags
    enum class TextureUsage : uint32_t
    {
        None                   = 0,
        TransferSrc            = 1 << 0,
        TransferDst            = 1 << 1,
        Sampled                = 1 << 2,
        Storage                = 1 << 3,
        ColorAttachment        = 1 << 4,
        DepthStencilAttachment = 1 << 5,
        TransientAttachment    = 1 << 6,
        InputAttachment        = 1 << 7,
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline TextureUsage operator&(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool hasFlag(TextureUsage flags, TextureUsage flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    // Memory properties
    enum class MemoryProperty : uint32_t
    {
        None            = 0,
        DeviceLocal     = 1 << 0,
        HostVisible     = 1 << 1,
        HostCoherent    = 1 << 2,
        HostCached      = 1 << 3,
        LazilyAllocated = 1 << 4,
    };

    inline MemoryProperty operator|(MemoryProperty a, MemoryProperty b)
    {
        return static_cast<MemoryProperty>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline MemoryProperty operator&(MemoryProperty a, MemoryProperty b)
    {
        return static_cast<MemoryProperty>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool hasFlag(MemoryProperty flags, MemoryProperty flag)
    {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
    }

    // Resource state for render graph
    enum class ResourceState
    {
        Undefined,
        General,
        CopySrc,
        CopyDst,
        ShaderRead,
        ShaderWrite,
        ColorAttachment,
        DepthStencilRead,
        DepthStencilWrite,
        Present,
    };

    // Pipeline stage flags
    enum class PipelineStage : uint32_t
    {
        None                   = 0,
        TopOfPipe              = 1 << 0,
        DrawIndirect           = 1 << 1,
        VertexInput            = 1 << 2,
        VertexShader           = 1 << 3,
        TessellationControl    = 1 << 4,
        TessellationEvaluation = 1 << 5,
        GeometryShader         = 1 << 6,
        FragmentShader         = 1 << 7,
        EarlyFragmentTests     = 1 << 8,
        LateFragmentTests      = 1 << 9,
        ColorAttachmentOutput  = 1 << 10,
        ComputeShader          = 1 << 11,
        Transfer               = 1 << 12,
        BottomOfPipe           = 1 << 13,
        Host                   = 1 << 14,
        AllGraphics            = 1 << 15,
        AllCommands            = 1 << 16,
    };

    inline PipelineStage operator|(PipelineStage a, PipelineStage b)
    {
        return static_cast<PipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    // Shader stages
    enum class ShaderStage : uint32_t
    {
        None           = 0,
        Vertex         = 1 << 0,
        Fragment       = 1 << 1,
        Compute        = 1 << 2,
        Geometry       = 1 << 3,
        TessControl    = 1 << 4,
        TessEvaluation = 1 << 5,
    };

    inline ShaderStage operator|(ShaderStage a, ShaderStage b)
    {
        return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    // Index type
    enum class IndexType
    {
        Uint16,
        Uint32,
    };

    // Cull mode
    enum class CullMode
    {
        None,
        Front,
        Back,
        FrontAndBack,
    };

    // Polygon mode
    enum class PolygonMode
    {
        Fill,
        Line,
        Point,
    };

    // Front face
    enum class FrontFace
    {
        CounterClockwise,
        Clockwise,
    };

    // Compare operation
    enum class CompareOp
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always,
    };

    // Blend factor
    enum class BlendFactor
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
    };

    // Blend operation
    enum class BlendOp
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    // Queue type
    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
    };

    // Texture type
    enum class TextureType
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
    };

    // Sample count
    enum class SampleCount
    {
        Samples1  = 1,
        Samples2  = 2,
        Samples4  = 4,
        Samples8  = 8,
        Samples16 = 16,
        Samples32 = 32,
    };

    // Clear values
    struct ClearColorValue
    {
        float float32[4] = {0.0f, 0.0f, 0.0f, 1.0f};

        ClearColorValue() = default;

        ClearColorValue(float r, float g, float b, float a)
        {
            float32[0] = r;
            float32[1] = g;
            float32[2] = b;
            float32[3] = a;
        }
    };

    struct ClearDepthStencilValue
    {
        float    depth   = 1.0f;
        uint32_t stencil = 0;
    };

    using ClearValue = std::variant<ClearColorValue, ClearDepthStencilValue>;

    // Subresource range
    struct TextureSubresourceRange
    {
        uint32_t baseMipLevel   = 0;
        uint32_t levelCount     = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount     = 1;
    };

    // Device capabilities
    struct DeviceCapabilities
    {
        uint32_t maxImageDimension1D            = 0;
        uint32_t maxImageDimension2D            = 0;
        uint32_t maxImageDimension3D            = 0;
        uint32_t maxImageDimensionCube          = 0;
        uint32_t maxUniformBufferRange          = 0;
        uint32_t maxStorageBufferRange          = 0;
        uint32_t maxPushConstantsSize           = 0;
        uint32_t maxMemoryAllocationCount       = 0;
        uint32_t maxSamplerAllocationCount      = 0;
        uint32_t maxBoundDescriptorSets         = 0;
        uint32_t maxComputeSharedMemorySize     = 0;
        uint32_t maxComputeWorkGroupCount[3]    = {};
        uint32_t maxComputeWorkGroupSize[3]     = {};
        uint32_t maxComputeWorkGroupInvocations = 0;
    };

    // Vertex input rate
    enum class VertexInputRate
    {
        Vertex,
        Instance,
    };

    // Primitive topology
    enum class PrimitiveTopology
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        LineListWithAdjacency,
        LineStripWithAdjacency,
        TriangleListWithAdjacency,
        TriangleStripWithAdjacency,
        PatchList,
    };
} // namespace engine::rhi
