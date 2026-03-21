#pragma once

#include <optional>
#include <string>
#include "Buffer.hpp"
#include "Core.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"

namespace engine::rhi
{
    // Forward declarations
    class DescriptorSet;
    class SwapChain;
    class Semaphore;
    class Fence;
    using DescriptorSetHandle = std::shared_ptr<DescriptorSet>;
    using SwapChainHandle     = std::shared_ptr<SwapChain>;
    using SemaphoreHandle     = std::shared_ptr<Semaphore>;
    using FenceHandle         = std::shared_ptr<Fence>;

    // Render pass attachment info
    struct RenderPassColorAttachment
    {
        TextureViewHandle              view;
        TextureViewHandle              resolveView; // For MSAA
        std::optional<ClearColorValue> clearValue;
        // Load/store ops determined by render graph
    };

    struct RenderPassDepthStencilAttachment
    {
        TextureViewHandle                     view;
        std::optional<ClearDepthStencilValue> clearValue;
        // Load/store ops determined by render graph
    };

    // Render pass begin info (for dynamic rendering)
    struct RenderPassBeginInfo
    {
        std::vector<RenderPassColorAttachment>          colorAttachments;
        std::optional<RenderPassDepthStencilAttachment> depthStencilAttachment;
        Rect2D                                          renderArea;
    };

    // Draw parameters
    struct DrawParams
    {
        uint32_t vertexCount   = 0;
        uint32_t instanceCount = 1;
        uint32_t firstVertex   = 0;
        uint32_t firstInstance = 0;
    };

    struct DrawIndexedParams
    {
        uint32_t indexCount    = 0;
        uint32_t instanceCount = 1;
        uint32_t firstIndex    = 0;
        int32_t  vertexOffset  = 0;
        uint32_t firstInstance = 0;
    };

    // Indirect draw parameters (std140 layout)
    struct DrawIndirectCommand
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct DrawIndexedIndirectCommand
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t  vertexOffset;
        uint32_t firstInstance;
    };

    // Dispatch parameters
    struct DispatchParams
    {
        uint32_t groupCountX = 1;
        uint32_t groupCountY = 1;
        uint32_t groupCountZ = 1;
    };

    // Resource barriers (called by render graph)
    struct BufferBarrier
    {
        BufferHandle  buffer;
        ResourceState srcState = ResourceState::Undefined;
        ResourceState dstState = ResourceState::Undefined;
        uint32_t      offset   = 0;
        uint32_t      size     = 0; // 0 = entire buffer
    };

    struct TextureBarrier
    {
        TextureHandle           texture;
        ResourceState           srcState = ResourceState::Undefined;
        ResourceState           dstState = ResourceState::Undefined;
        TextureSubresourceRange subresourceRange;

        TextureBarrier() = default;

        TextureBarrier(TextureHandle tex, ResourceState src, ResourceState dst)
            : texture(std::move(tex))
            , srcState(src)
            , dstState(dst)
        {
        }
    };

    // Descriptor set class (simplified for now)
    class DescriptorSet
    {
        public:
            DescriptorSet() = default;
            ~DescriptorSet();

            DescriptorSet(const DescriptorSet&)            = delete;
            DescriptorSet& operator=(const DescriptorSet&) = delete;

            DescriptorSet(DescriptorSet&& other) noexcept;
            DescriptorSet& operator=(DescriptorSet&& other) noexcept;

            [[nodiscard]] bool                      isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkDescriptorSet           nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] DescriptorSetLayoutHandle layout() const { return layout_; }

            // Write operations would be added here

            struct InternalData
            {
                VkDescriptorSet           set    = nullptr;
                VkDevice                  device = nullptr;
                DescriptorSetLayoutHandle layout;
            };

            explicit DescriptorSet(const InternalData& data);
            void     release();

        private:
            VkDescriptorSet           handle_ = nullptr;
            VkDevice                  device_ = nullptr;
            DescriptorSetLayoutHandle layout_;
    };

    using DescriptorSetHandle = std::shared_ptr<DescriptorSet>;

    // Command buffer class
    class CommandBuffer
    {
        public:
            CommandBuffer() = default;
            ~CommandBuffer();

            CommandBuffer(const CommandBuffer&)            = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;

            CommandBuffer(CommandBuffer&& other) noexcept;
            CommandBuffer& operator=(CommandBuffer&& other) noexcept;

            [[nodiscard]] bool            isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkCommandBuffer nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] bool            isRecording() const noexcept { return isRecording_; }

            // Recording control
            Result begin();
            Result end();
            Result reset();

            // Resource barriers (called by render graph)
            void bufferBarrier(const BufferBarrier& barrier);
            void bufferBarriers(const std::vector<BufferBarrier>& barriers);
            void textureBarrier(const TextureBarrier& barrier);
            void textureBarriers(const std::vector<TextureBarrier>& barriers);
            void barrier(
                const std::vector<BufferBarrier>&  bufferBarriers,
                const std::vector<TextureBarrier>& textureBarriers);

            // Memory barrier shortcut (UAV barrier equivalent)
            void memoryBarrier(PipelineStage srcStage, PipelineStage dstStage);

            // Render pass (dynamic rendering)
            void beginRenderPass(const RenderPassBeginInfo& info);
            void endRenderPass();

            // Pipeline binding
            void bindPipeline(GraphicsPipelineHandle pipeline);
            void bindPipeline(ComputePipelineHandle pipeline);

            // Descriptor set binding
            void bindDescriptorSet(
                PipelineLayoutHandle         layout,
                uint32_t                     setIndex,
                DescriptorSetHandle          set,
                const std::vector<uint32_t>& dynamicOffsets = {});
            void bindDescriptorSets(
                PipelineLayoutHandle                    layout,
                uint32_t                                firstSet,
                const std::vector<DescriptorSetHandle>& sets,
                const std::vector<uint32_t>&            dynamicOffsets = {});

            // Push constants
            void pushConstants(
                PipelineLayoutHandle layout,
                ShaderStage          stages,
                uint32_t             offset,
                uint32_t             size,
                const void*          data);

            // Vertex/index buffers
            void bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint32_t offset = 0);
            void bindVertexBuffers(
                uint32_t                         firstBinding,
                const std::vector<BufferHandle>& buffers,
                const std::vector<uint32_t>&     offsets);
            void bindIndexBuffer(BufferHandle buffer, IndexType type, uint32_t offset = 0);

            // Viewport and scissor
            void setViewport(const Viewport& viewport);
            void setViewports(const std::vector<Viewport>& viewports);
            void setScissor(const Rect2D& scissor);
            void setScissors(const std::vector<Rect2D>& scissors);

            // Line width (if not dynamic)
            void setLineWidth(float width);

            // Depth bias (if not dynamic)
            void setDepthBias(float constantFactor, float clamp, float slopeFactor);

            // Blend constants (if not dynamic)
            void setBlendConstants(const float blendConstants[4]);

            // Depth bounds (if enabled)
            void setDepthBounds(float minDepth, float maxDepth);

            // Stencil reference (if not dynamic)
            void setStencilReference(uint32_t reference);

            // Draw commands
            void draw(const DrawParams& params);
            void drawIndexed(const DrawIndexedParams& params);
            void drawIndirect(BufferHandle buffer, uint32_t offset, uint32_t drawCount, uint32_t stride);
            void drawIndexedIndirect(BufferHandle buffer, uint32_t offset, uint32_t drawCount, uint32_t stride);

            // Compute dispatch
            void dispatch(const DispatchParams& params);
            void dispatchIndirect(BufferHandle buffer, uint32_t offset);

            // Resource copies
            void copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region);
            void copyBuffer(BufferHandle src, BufferHandle dst, const std::vector<BufferCopyRegion>& regions);
            void copyBufferToTexture(BufferHandle src, TextureHandle dst, const BufferTextureCopyRegion& region);
            void copyBufferToTexture(BufferHandle src, TextureHandle dst, const std::vector<BufferTextureCopyRegion>& regions);
            void copyTextureToBuffer(TextureHandle src, BufferHandle dst, const BufferTextureCopyRegion& region);
            void copyTextureToBuffer(TextureHandle src, BufferHandle dst, const std::vector<BufferTextureCopyRegion>& regions);
            void copyTexture(TextureHandle src, TextureHandle dst, const TextureCopyRegion& region);
            void copyTexture(TextureHandle src, TextureHandle dst, const std::vector<TextureCopyRegion>& regions);

            // Clears
            void clearColorTexture(
                TextureHandle                  texture,
                const ClearColorValue&         value,
                const TextureSubresourceRange& range);
            void clearDepthStencilTexture(
                TextureHandle                  texture,
                const ClearDepthStencilValue&  value,
                const TextureSubresourceRange& range);
            void fillBuffer(BufferHandle buffer, uint32_t offset, uint32_t size, uint32_t data);
            void updateBuffer(BufferHandle buffer, uint32_t offset, uint32_t size, const void* data);

            // Debug markers (if debug utils enabled)
            void beginDebugLabel(const std::string& label, const float color[4]);
            void endDebugLabel();
            void insertDebugMarker(const std::string& label, const float color[4]);

            // Internal construction
            struct InternalData
            {
                VkCommandBuffer cmd       = nullptr;
                VkDevice        device    = nullptr;
                VkCommandPool   pool      = nullptr;
                QueueType       queueType = QueueType::Graphics;
            };

            explicit CommandBuffer(const InternalData& data);
            void     release();

        private:
            VkCommandBuffer handle_           = nullptr;
            VkDevice        device_           = nullptr;
            VkCommandPool   pool_             = nullptr;
            QueueType       queueType_        = QueueType::Graphics;
            bool            isRecording_      = false;
            bool            insideRenderPass_ = false;

            // Currently bound state (for validation if needed)
            GraphicsPipelineHandle boundGraphicsPipeline_;
            ComputePipelineHandle  boundComputePipeline_;
    };

    using CommandBufferHandle = std::shared_ptr<CommandBuffer>;
} // namespace engine::rhi
