#include "engine/rhi/CommandBuffer.hpp"
#include <vulkan/vulkan.h>
#include <cstring>

namespace engine::rhi
{
    // DescriptorSet implementation
    DescriptorSet::~DescriptorSet()
    {
        release();
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , layout_(std::move(other.layout_))
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            layout_       = std::move(other.layout_);
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    DescriptorSet::DescriptorSet(const InternalData& data)
        : handle_(data.set)
        , device_(data.device)
        , layout_(data.layout)
    {
    }

    void DescriptorSet::release()
    {
        // Descriptor sets are freed with the pool, nothing to do here
        handle_ = nullptr;
        device_ = nullptr;
        layout_.reset();
    }

    // CommandBuffer implementation
    CommandBuffer::~CommandBuffer()
    {
        release();
    }

    CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , pool_(other.pool_)
        , queueType_(other.queueType_)
        , isRecording_(other.isRecording_)
        , insideRenderPass_(other.insideRenderPass_)
        , boundGraphicsPipeline_(std::move(other.boundGraphicsPipeline_))
        , boundComputePipeline_(std::move(other.boundComputePipeline_))
    {
        other.handle_           = nullptr;
        other.device_           = nullptr;
        other.pool_             = nullptr;
        other.isRecording_      = false;
        other.insideRenderPass_ = false;
    }

    CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_                 = other.handle_;
            device_                 = other.device_;
            pool_                   = other.pool_;
            queueType_              = other.queueType_;
            isRecording_            = other.isRecording_;
            insideRenderPass_       = other.insideRenderPass_;
            boundGraphicsPipeline_  = std::move(other.boundGraphicsPipeline_);
            boundComputePipeline_   = std::move(other.boundComputePipeline_);
            other.handle_           = nullptr;
            other.device_           = nullptr;
            other.pool_             = nullptr;
            other.isRecording_      = false;
            other.insideRenderPass_ = false;
        }
        return *this;
    }

    CommandBuffer::CommandBuffer(const InternalData& data)
        : handle_(data.cmd)
        , device_(data.device)
        , pool_(data.pool)
        , queueType_(data.queueType)
    {
    }

    void CommandBuffer::release()
    {
        // Command buffers are freed with the pool, nothing to do here
        handle_           = nullptr;
        device_           = nullptr;
        pool_             = nullptr;
        isRecording_      = false;
        insideRenderPass_ = false;
        boundGraphicsPipeline_.reset();
        boundComputePipeline_.reset();
    }

    Result CommandBuffer::begin()
    {
        if (!handle_) return Result::Error_InvalidParameter;
        if (isRecording_) return Result::Error_InvalidParameter;

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult result = vkBeginCommandBuffer(handle_, &beginInfo);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        isRecording_      = true;
        insideRenderPass_ = false;
        boundGraphicsPipeline_.reset();
        boundComputePipeline_.reset();

        return Result::Success;
    }

    Result CommandBuffer::end()
    {
        if (!handle_) return Result::Error_InvalidParameter;
        if (!isRecording_) return Result::Error_InvalidParameter;
        if (insideRenderPass_) return Result::Error_InvalidParameter;

        VkResult result = vkEndCommandBuffer(handle_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_InvalidParameter;
        }

        isRecording_ = false;
        return Result::Success;
    }

    Result CommandBuffer::reset()
    {
        if (!handle_) return Result::Error_InvalidParameter;

        VkResult result = vkResetCommandBuffer(handle_, 0);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        isRecording_      = false;
        insideRenderPass_ = false;
        boundGraphicsPipeline_.reset();
        boundComputePipeline_.reset();

        return Result::Success;
    }

    // Helper to convert ResourceState to Vulkan stage and access flags
    static void getStageAndAccess(ResourceState state, VkPipelineStageFlags& stage, VkAccessFlags& access)
    {
        switch (state)
        {
            case ResourceState::Undefined:
                stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                access = 0;
                break;
            case ResourceState::General:
                stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                break;
            case ResourceState::CopySrc:
                stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                access = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case ResourceState::CopyDst:
                stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                access = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case ResourceState::ShaderRead:
                stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                access = VK_ACCESS_SHADER_READ_BIT;
                break;
            case ResourceState::ShaderWrite:
                stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                access = VK_ACCESS_SHADER_WRITE_BIT;
                break;
            case ResourceState::ColorAttachment:
                stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case ResourceState::DepthStencilRead:
                stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;
            case ResourceState::DepthStencilWrite:
                stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case ResourceState::Present:
                stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                access = 0;
                break;
            default:
                stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        }
    }

    void CommandBuffer::bufferBarrier(const BufferBarrier& barrier)
    {
        if (!handle_ || !barrier.buffer) return;

        VkPipelineStageFlags srcStage,  dstStage;
        VkAccessFlags        srcAccess, dstAccess;
        getStageAndAccess(barrier.srcState, srcStage, srcAccess);
        getStageAndAccess(barrier.dstState, dstStage, dstAccess);

        VkBufferMemoryBarrier vkBarrier = {};
        vkBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkBarrier.srcAccessMask         = srcAccess;
        vkBarrier.dstAccessMask         = dstAccess;
        vkBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.buffer                = barrier.buffer->nativeHandle();
        vkBarrier.offset                = barrier.offset;
        vkBarrier.size                  = barrier.size == 0 ? VK_WHOLE_SIZE : barrier.size;

        vkCmdPipelineBarrier(handle_, srcStage, dstStage, 0, 0, nullptr, 1, &vkBarrier, 0, nullptr);
    }

    void CommandBuffer::bufferBarriers(const std::vector<BufferBarrier>& barriers)
    {
        if (!handle_ || barriers.empty()) return;

        for (const auto& barrier : barriers)
        {
            bufferBarrier(barrier);
        }
    }

    void CommandBuffer::textureBarrier(const TextureBarrier& barrier)
    {
        if (!handle_ || !barrier.texture) return;

        VkPipelineStageFlags srcStage,  dstStage;
        VkAccessFlags        srcAccess, dstAccess;
        getStageAndAccess(barrier.srcState, srcStage, srcAccess);
        getStageAndAccess(barrier.dstState, dstStage, dstAccess);

        // Determine layout transition
        VkImageLayout oldLayout, newLayout;
        auto          getLayout = [](ResourceState state) -> VkImageLayout
        {
            switch (state)
            {
                case ResourceState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
                case ResourceState::General: return VK_IMAGE_LAYOUT_GENERAL;
                case ResourceState::CopySrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                case ResourceState::CopyDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                case ResourceState::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                case ResourceState::ShaderWrite: return VK_IMAGE_LAYOUT_GENERAL;
                case ResourceState::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                case ResourceState::DepthStencilRead: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                case ResourceState::DepthStencilWrite: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                case ResourceState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                default: return VK_IMAGE_LAYOUT_UNDEFINED;
            }
        };

        oldLayout = getLayout(barrier.srcState);
        newLayout = getLayout(barrier.dstState);

        VkImageMemoryBarrier vkBarrier            = {};
        vkBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkBarrier.srcAccessMask                   = srcAccess;
        vkBarrier.dstAccessMask                   = dstAccess;
        vkBarrier.oldLayout                       = oldLayout;
        vkBarrier.newLayout                       = newLayout;
        vkBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.image                           = barrier.texture->nativeHandle();
        vkBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vkBarrier.subresourceRange.baseMipLevel   = barrier.subresourceRange.baseMipLevel;
        vkBarrier.subresourceRange.levelCount     = barrier.subresourceRange.levelCount;
        vkBarrier.subresourceRange.baseArrayLayer = barrier.subresourceRange.baseArrayLayer;
        vkBarrier.subresourceRange.layerCount     = barrier.subresourceRange.layerCount;

        vkCmdPipelineBarrier(handle_, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &vkBarrier);
    }

    void CommandBuffer::textureBarriers(const std::vector<TextureBarrier>& barriers)
    {
        if (!handle_ || barriers.empty()) return;

        for (const auto& barrier : barriers)
        {
            textureBarrier(barrier);
        }
    }

    void CommandBuffer::barrier(
        const std::vector<BufferBarrier>&  bufferBarriers,
        const std::vector<TextureBarrier>& textureBarriers)
    {
        this->bufferBarriers(bufferBarriers);
        this->textureBarriers(textureBarriers);
    }

    void CommandBuffer::memoryBarrier(PipelineStage srcStage, PipelineStage dstStage)
    {
        if (!handle_) return;

        VkPipelineStageFlags src = static_cast<VkPipelineStageFlags>(srcStage);
        VkPipelineStageFlags dst = static_cast<VkPipelineStageFlags>(dstStage);

        VkMemoryBarrier barrier = {};
        barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask   = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

        vkCmdPipelineBarrier(handle_, src, dst, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    void CommandBuffer::beginRenderPass(const RenderPassBeginInfo& info)
    {
        if (!handle_) return;
        if (insideRenderPass_) return;

        // Build rendering info for dynamic rendering
        std::vector<VkRenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(info.colorAttachments.size());

        for (const auto& attachment : info.colorAttachments)
        {
            if (!attachment.view) continue;

            VkRenderingAttachmentInfo vkAttachment = {};
            vkAttachment.sType                     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            vkAttachment.imageView                 = attachment.view->nativeHandle();
            vkAttachment.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vkAttachment.resolveMode               = VK_RESOLVE_MODE_NONE;

            if (attachment.clearValue)
            {
                vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                const auto& clear   = attachment.clearValue.value();
                std::memcpy(vkAttachment.clearValue.color.float32, clear.float32, sizeof(clear.float32));
            }
            else
            {
                vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            vkAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            colorAttachments.push_back(vkAttachment);
        }

        VkRenderingAttachmentInfo  depthAttachment  = {};
        VkRenderingAttachmentInfo* pDepthAttachment = nullptr;

        if (info.depthStencilAttachment && info.depthStencilAttachment->view)
        {
            depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView   = info.depthStencilAttachment->view->nativeHandle();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            if (info.depthStencilAttachment->clearValue)
            {
                depthAttachment.loadOp                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
                const auto& clear                               = info.depthStencilAttachment->clearValue.value();
                depthAttachment.clearValue.depthStencil.depth   = clear.depth;
                depthAttachment.clearValue.depthStencil.stencil = clear.stencil;
            }
            else
            {
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            pDepthAttachment        = &depthAttachment;
        }

        VkRenderingInfo renderingInfo      = {};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset    = {info.renderArea.offset.x, info.renderArea.offset.y};
        renderingInfo.renderArea.extent    = {info.renderArea.extent.width, info.renderArea.extent.height};
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
        renderingInfo.pColorAttachments    = colorAttachments.empty() ? nullptr : colorAttachments.data();
        renderingInfo.pDepthAttachment     = pDepthAttachment;
        renderingInfo.pStencilAttachment   = nullptr;

        vkCmdBeginRendering(handle_, &renderingInfo);
        insideRenderPass_ = true;
    }

    void CommandBuffer::endRenderPass()
    {
        if (!handle_) return;
        if (!insideRenderPass_) return;

        vkCmdEndRendering(handle_);
        insideRenderPass_ = false;
    }

    void CommandBuffer::bindPipeline(GraphicsPipelineHandle pipeline)
    {
        if (!handle_ || !pipeline) return;

        vkCmdBindPipeline(handle_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->nativeHandle());
        boundGraphicsPipeline_ = pipeline;
        boundComputePipeline_.reset();
    }

    void CommandBuffer::bindPipeline(ComputePipelineHandle pipeline)
    {
        if (!handle_ || !pipeline) return;

        vkCmdBindPipeline(handle_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->nativeHandle());
        boundComputePipeline_ = pipeline;
        boundGraphicsPipeline_.reset();
    }

    void CommandBuffer::bindDescriptorSet(
        PipelineLayoutHandle         layout,
        uint32_t                     setIndex,
        DescriptorSetHandle          set,
        const std::vector<uint32_t>& dynamicOffsets)
    {
        if (!handle_ || !layout || !set) return;

        VkPipelineBindPoint bindPoint = boundGraphicsPipeline_ ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

        VkDescriptorSet vkSet = set->nativeHandle();

        vkCmdBindDescriptorSets(handle_,
                                bindPoint,
                                layout->nativeHandle(),
                                setIndex,
                                1,
                                &vkSet,
                                static_cast<uint32_t>(dynamicOffsets.size()),
                                dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
    }

    void CommandBuffer::bindDescriptorSets(
        PipelineLayoutHandle                    layout,
        uint32_t                                firstSet,
        const std::vector<DescriptorSetHandle>& sets,
        const std::vector<uint32_t>&            dynamicOffsets)
    {
        if (!handle_ || !layout || sets.empty()) return;

        VkPipelineBindPoint bindPoint = boundGraphicsPipeline_ ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

        std::vector<VkDescriptorSet> vkSets;
        vkSets.reserve(sets.size());
        for (const auto& set : sets)
        {
            if (set) vkSets.push_back(set->nativeHandle());
        }

        vkCmdBindDescriptorSets(handle_,
                                bindPoint,
                                layout->nativeHandle(),
                                firstSet,
                                static_cast<uint32_t>(vkSets.size()),
                                vkSets.data(),
                                static_cast<uint32_t>(dynamicOffsets.size()),
                                dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
    }

    void CommandBuffer::pushConstants(
        PipelineLayoutHandle layout,
        ShaderStage          stages,
        uint32_t             offset,
        uint32_t             size,
        const void*          data)
    {
        if (!handle_ || !layout || !data || size == 0) return;

        VkShaderStageFlags vkStages = 0;
        if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(ShaderStage::Vertex))
            vkStages |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(ShaderStage::Fragment))
            vkStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(ShaderStage::Compute))
            vkStages |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (static_cast<uint32_t>(stages) & static_cast<uint32_t>(ShaderStage::Geometry))
            vkStages |= VK_SHADER_STAGE_GEOMETRY_BIT;

        vkCmdPushConstants(handle_, layout->nativeHandle(), vkStages, offset, size, data);
    }

    void CommandBuffer::bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint32_t offset)
    {
        if (!handle_ || !buffer) return;

        VkBuffer     vkBuffer = buffer->nativeHandle();
        VkDeviceSize vkOffset = offset;

        vkCmdBindVertexBuffers(handle_, binding, 1, &vkBuffer, &vkOffset);
    }

    void CommandBuffer::bindVertexBuffers(
        uint32_t                         firstBinding,
        const std::vector<BufferHandle>& buffers,
        const std::vector<uint32_t>&     offsets)
    {
        if (!handle_ || buffers.empty()) return;

        std::vector<VkBuffer>     vkBuffers;
        std::vector<VkDeviceSize> vkOffsets;

        vkBuffers.reserve(buffers.size());
        vkOffsets.reserve(offsets.size());

        for (size_t i = 0; i < buffers.size(); ++i)
        {
            if (buffers[i])
            {
                vkBuffers.push_back(buffers[i]->nativeHandle());
                vkOffsets.push_back(i < offsets.size() ? offsets[i] : 0);
            }
        }

        vkCmdBindVertexBuffers(handle_,
                               firstBinding,
                               static_cast<uint32_t>(vkBuffers.size()),
                               vkBuffers.data(),
                               vkOffsets.data());
    }

    void CommandBuffer::bindIndexBuffer(BufferHandle buffer, IndexType type, uint32_t offset)
    {
        if (!handle_ || !buffer) return;

        VkIndexType vkType = (type == IndexType::Uint32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
        vkCmdBindIndexBuffer(handle_, buffer->nativeHandle(), offset, vkType);
    }

    void CommandBuffer::setViewport(const Viewport& viewport)
    {
        if (!handle_) return;

        VkViewport vkViewport = {};
        vkViewport.x          = viewport.x;
        vkViewport.y          = viewport.y;
        vkViewport.width      = viewport.width;
        vkViewport.height     = viewport.height;
        vkViewport.minDepth   = viewport.minDepth;
        vkViewport.maxDepth   = viewport.maxDepth;

        vkCmdSetViewport(handle_, 0, 1, &vkViewport);
    }

    void CommandBuffer::setViewports(const std::vector<Viewport>& viewports)
    {
        if (!handle_ || viewports.empty()) return;

        std::vector<VkViewport> vkViewports;
        vkViewports.reserve(viewports.size());

        for (const auto& vp : viewports)
        {
            VkViewport vkViewport = {};
            vkViewport.x          = vp.x;
            vkViewport.y          = vp.y;
            vkViewport.width      = vp.width;
            vkViewport.height     = vp.height;
            vkViewport.minDepth   = vp.minDepth;
            vkViewport.maxDepth   = vp.maxDepth;
            vkViewports.push_back(vkViewport);
        }

        vkCmdSetViewport(handle_, 0, static_cast<uint32_t>(vkViewports.size()), vkViewports.data());
    }

    void CommandBuffer::setScissor(const Rect2D& scissor)
    {
        if (!handle_) return;

        VkRect2D vkScissor = {};
        vkScissor.offset   = {scissor.offset.x, scissor.offset.y};
        vkScissor.extent   = {scissor.extent.width, scissor.extent.height};

        vkCmdSetScissor(handle_, 0, 1, &vkScissor);
    }

    void CommandBuffer::setScissors(const std::vector<Rect2D>& scissors)
    {
        if (!handle_ || scissors.empty()) return;

        std::vector<VkRect2D> vkScissors;
        vkScissors.reserve(scissors.size());

        for (const auto& sc : scissors)
        {
            VkRect2D vkScissor = {};
            vkScissor.offset   = {sc.offset.x, sc.offset.y};
            vkScissor.extent   = {sc.extent.width, sc.extent.height};
            vkScissors.push_back(vkScissor);
        }

        vkCmdSetScissor(handle_, 0, static_cast<uint32_t>(vkScissors.size()), vkScissors.data());
    }

    void CommandBuffer::setLineWidth(float width)
    {
        if (!handle_) return;
        vkCmdSetLineWidth(handle_, width);
    }

    void CommandBuffer::setDepthBias(float constantFactor, float clamp, float slopeFactor)
    {
        if (!handle_) return;
        vkCmdSetDepthBias(handle_, constantFactor, clamp, slopeFactor);
    }

    void CommandBuffer::setBlendConstants(const float blendConstants[4])
    {
        if (!handle_) return;
        vkCmdSetBlendConstants(handle_, blendConstants);
    }

    void CommandBuffer::setDepthBounds(float minDepth, float maxDepth)
    {
        if (!handle_) return;
        vkCmdSetDepthBounds(handle_, minDepth, maxDepth);
    }

    void CommandBuffer::setStencilReference(uint32_t reference)
    {
        if (!handle_) return;
        vkCmdSetStencilReference(handle_, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
    }

    void CommandBuffer::draw(const DrawParams& params)
    {
        if (!handle_) return;
        vkCmdDraw(handle_,
                  params.vertexCount,
                  params.instanceCount,
                  params.firstVertex,
                  params.firstInstance);
    }

    void CommandBuffer::drawIndexed(const DrawIndexedParams& params)
    {
        if (!handle_) return;
        vkCmdDrawIndexed(handle_,
                         params.indexCount,
                         params.instanceCount,
                         params.firstIndex,
                         params.vertexOffset,
                         params.firstInstance);
    }

    void CommandBuffer::drawIndirect(BufferHandle buffer, uint32_t offset, uint32_t drawCount, uint32_t stride)
    {
        if (!handle_ || !buffer) return;
        vkCmdDrawIndirect(handle_, buffer->nativeHandle(), offset, drawCount, stride);
    }

    void CommandBuffer::drawIndexedIndirect(BufferHandle buffer, uint32_t offset, uint32_t drawCount, uint32_t stride)
    {
        if (!handle_ || !buffer) return;
        vkCmdDrawIndexedIndirect(handle_, buffer->nativeHandle(), offset, drawCount, stride);
    }

    void CommandBuffer::dispatch(const DispatchParams& params)
    {
        if (!handle_) return;
        vkCmdDispatch(handle_, params.groupCountX, params.groupCountY, params.groupCountZ);
    }

    void CommandBuffer::dispatchIndirect(BufferHandle buffer, uint32_t offset)
    {
        if (!handle_ || !buffer) return;
        vkCmdDispatchIndirect(handle_, buffer->nativeHandle(), offset);
    }

    void CommandBuffer::copyBuffer(BufferHandle src, BufferHandle dst, const BufferCopyRegion& region)
    {
        if (!handle_ || !src || !dst) return;

        VkBufferCopy copy = {};
        copy.srcOffset    = region.srcOffset;
        copy.dstOffset    = region.dstOffset;
        copy.size         = region.size;

        vkCmdCopyBuffer(handle_, src->nativeHandle(), dst->nativeHandle(), 1, &copy);
    }

    void CommandBuffer::copyBuffer(BufferHandle src, BufferHandle dst, const std::vector<BufferCopyRegion>& regions)
    {
        if (!handle_ || !src || !dst || regions.empty()) return;

        std::vector<VkBufferCopy> copies;
        copies.reserve(regions.size());

        for (const auto& region : regions)
        {
            VkBufferCopy copy = {};
            copy.srcOffset    = region.srcOffset;
            copy.dstOffset    = region.dstOffset;
            copy.size         = region.size;
            copies.push_back(copy);
        }

        vkCmdCopyBuffer(handle_,
                        src->nativeHandle(),
                        dst->nativeHandle(),
                        static_cast<uint32_t>(copies.size()),
                        copies.data());
    }

    void CommandBuffer::copyBufferToTexture(BufferHandle src, TextureHandle dst, const BufferTextureCopyRegion& region)
    {
        if (!handle_ || !src || !dst) return;

        VkBufferImageCopy copy               = {};
        copy.bufferOffset                    = region.bufferOffset;
        copy.bufferRowLength                 = region.bufferRowLength;
        copy.bufferImageHeight               = region.bufferImageHeight;
        copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel       = region.imageSubresource.baseMipLevel;
        copy.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer;
        copy.imageSubresource.layerCount     = region.imageSubresource.layerCount;
        copy.imageOffset                     = {region.imageOffset.x, region.imageOffset.y, region.imageOffset.z};
        copy.imageExtent                     = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};

        vkCmdCopyBufferToImage(handle_,
                               src->nativeHandle(),
                               dst->nativeHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copy);
    }

    void CommandBuffer::copyBufferToTexture(
        BufferHandle                                src,
        TextureHandle                               dst,
        const std::vector<BufferTextureCopyRegion>& regions)
    {
        if (!handle_ || !src || !dst || regions.empty()) return;

        std::vector<VkBufferImageCopy> copies;
        copies.reserve(regions.size());

        for (const auto& region : regions)
        {
            VkBufferImageCopy copy               = {};
            copy.bufferOffset                    = region.bufferOffset;
            copy.bufferRowLength                 = region.bufferRowLength;
            copy.bufferImageHeight               = region.bufferImageHeight;
            copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel       = region.imageSubresource.baseMipLevel;
            copy.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer;
            copy.imageSubresource.layerCount     = region.imageSubresource.layerCount;
            copy.imageOffset                     = {region.imageOffset.x, region.imageOffset.y, region.imageOffset.z};
            copy.imageExtent                     = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};
            copies.push_back(copy);
        }

        vkCmdCopyBufferToImage(handle_,
                               src->nativeHandle(),
                               dst->nativeHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(copies.size()),
                               copies.data());
    }

    void CommandBuffer::copyTextureToBuffer(TextureHandle src, BufferHandle dst, const BufferTextureCopyRegion& region)
    {
        if (!handle_ || !src || !dst) return;

        VkBufferImageCopy copy               = {};
        copy.bufferOffset                    = region.bufferOffset;
        copy.bufferRowLength                 = region.bufferRowLength;
        copy.bufferImageHeight               = region.bufferImageHeight;
        copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel       = region.imageSubresource.baseMipLevel;
        copy.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer;
        copy.imageSubresource.layerCount     = region.imageSubresource.layerCount;
        copy.imageOffset                     = {region.imageOffset.x, region.imageOffset.y, region.imageOffset.z};
        copy.imageExtent                     = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};

        vkCmdCopyImageToBuffer(handle_,
                               src->nativeHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               dst->nativeHandle(),
                               1,
                               &copy);
    }

    void CommandBuffer::copyTextureToBuffer(
        TextureHandle                               src,
        BufferHandle                                dst,
        const std::vector<BufferTextureCopyRegion>& regions)
    {
        if (!handle_ || !src || !dst || regions.empty()) return;

        std::vector<VkBufferImageCopy> copies;
        copies.reserve(regions.size());

        for (const auto& region : regions)
        {
            VkBufferImageCopy copy               = {};
            copy.bufferOffset                    = region.bufferOffset;
            copy.bufferRowLength                 = region.bufferRowLength;
            copy.bufferImageHeight               = region.bufferImageHeight;
            copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel       = region.imageSubresource.baseMipLevel;
            copy.imageSubresource.baseArrayLayer = region.imageSubresource.baseArrayLayer;
            copy.imageSubresource.layerCount     = region.imageSubresource.layerCount;
            copy.imageOffset                     = {region.imageOffset.x, region.imageOffset.y, region.imageOffset.z};
            copy.imageExtent                     = {region.imageExtent.width, region.imageExtent.height, region.imageExtent.depth};
            copies.push_back(copy);
        }

        vkCmdCopyImageToBuffer(handle_,
                               src->nativeHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               dst->nativeHandle(),
                               static_cast<uint32_t>(copies.size()),
                               copies.data());
    }

    void CommandBuffer::copyTexture(TextureHandle src, TextureHandle dst, const TextureCopyRegion& region)
    {
        if (!handle_ || !src || !dst) return;

        VkImageCopy copy                   = {};
        copy.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.srcSubresource.mipLevel       = region.srcSubresource.baseMipLevel;
        copy.srcSubresource.baseArrayLayer = region.srcSubresource.baseArrayLayer;
        copy.srcSubresource.layerCount     = region.srcSubresource.layerCount;
        copy.srcOffset                     = {region.srcOffset.x, region.srcOffset.y, region.srcOffset.z};
        copy.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.dstSubresource.mipLevel       = region.dstSubresource.baseMipLevel;
        copy.dstSubresource.baseArrayLayer = region.dstSubresource.baseArrayLayer;
        copy.dstSubresource.layerCount     = region.dstSubresource.layerCount;
        copy.dstOffset                     = {region.dstOffset.x, region.dstOffset.y, region.dstOffset.z};
        copy.extent                        = {region.extent.width, region.extent.height, region.extent.depth};

        vkCmdCopyImage(handle_,
                       src->nativeHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst->nativeHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &copy);
    }

    void CommandBuffer::copyTexture(TextureHandle src, TextureHandle dst, const std::vector<TextureCopyRegion>& regions)
    {
        if (!handle_ || !src || !dst || regions.empty()) return;

        std::vector<VkImageCopy> copies;
        copies.reserve(regions.size());

        for (const auto& region : regions)
        {
            VkImageCopy copy                   = {};
            copy.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.srcSubresource.mipLevel       = region.srcSubresource.baseMipLevel;
            copy.srcSubresource.baseArrayLayer = region.srcSubresource.baseArrayLayer;
            copy.srcSubresource.layerCount     = region.srcSubresource.layerCount;
            copy.srcOffset                     = {region.srcOffset.x, region.srcOffset.y, region.srcOffset.z};
            copy.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.dstSubresource.mipLevel       = region.dstSubresource.baseMipLevel;
            copy.dstSubresource.baseArrayLayer = region.dstSubresource.baseArrayLayer;
            copy.dstSubresource.layerCount     = region.dstSubresource.layerCount;
            copy.dstOffset                     = {region.dstOffset.x, region.dstOffset.y, region.dstOffset.z};
            copy.extent                        = {region.extent.width, region.extent.height, region.extent.depth};
            copies.push_back(copy);
        }

        vkCmdCopyImage(handle_,
                       src->nativeHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst->nativeHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       static_cast<uint32_t>(copies.size()),
                       copies.data());
    }

    void CommandBuffer::clearColorTexture(
        TextureHandle                  texture,
        const ClearColorValue&         value,
        const TextureSubresourceRange& range)
    {
        if (!handle_ || !texture) return;

        VkImageSubresourceRange vkRange = {};
        vkRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        vkRange.baseMipLevel            = range.baseMipLevel;
        vkRange.levelCount              = range.levelCount;
        vkRange.baseArrayLayer          = range.baseArrayLayer;
        vkRange.layerCount              = range.layerCount;

        VkClearColorValue vkValue = {};
        std::memcpy(vkValue.float32, value.float32, sizeof(value.float32));

        vkCmdClearColorImage(handle_,
                             texture->nativeHandle(),
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &vkValue,
                             1,
                             &vkRange);
    }

    void CommandBuffer::clearDepthStencilTexture(
        TextureHandle                  texture,
        const ClearDepthStencilValue&  value,
        const TextureSubresourceRange& range)
    {
        if (!handle_ || !texture) return;

        VkImageSubresourceRange vkRange = {};
        vkRange.aspectMask              = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        vkRange.baseMipLevel            = range.baseMipLevel;
        vkRange.levelCount              = range.levelCount;
        vkRange.baseArrayLayer          = range.baseArrayLayer;
        vkRange.layerCount              = range.layerCount;

        VkClearDepthStencilValue vkValue = {};
        vkValue.depth                    = value.depth;
        vkValue.stencil                  = value.stencil;

        vkCmdClearDepthStencilImage(handle_,
                                    texture->nativeHandle(),
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    &vkValue,
                                    1,
                                    &vkRange);
    }

    void CommandBuffer::fillBuffer(BufferHandle buffer, uint32_t offset, uint32_t size, uint32_t data)
    {
        if (!handle_ || !buffer) return;
        vkCmdFillBuffer(handle_,
                        buffer->nativeHandle(),
                        offset,
                        size == 0 ? VK_WHOLE_SIZE : size,
                        data);
    }

    void CommandBuffer::updateBuffer(BufferHandle buffer, uint32_t offset, uint32_t size, const void* data)
    {
        if (!handle_ || !buffer || !data || size == 0) return;
        if (size > 65536) return; // Vulkan limit for updateBuffer

        vkCmdUpdateBuffer(handle_, buffer->nativeHandle(), offset, size, data);
    }

    void CommandBuffer::beginDebugLabel(const std::string& label, const float color[4])
    {
        (void)label;
        (void)color;
    }

    void CommandBuffer::endDebugLabel()
    {
    }

    void CommandBuffer::insertDebugMarker(const std::string& label, const float color[4])
    {
        (void)label;
        (void)color;
    }
} // namespace engine::rhi
