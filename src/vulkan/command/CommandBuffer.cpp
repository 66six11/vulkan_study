#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <cstring>

namespace vulkan_engine::vulkan
{
    // RenderCommandBuffer implementation
    RenderCommandBuffer::RenderCommandBuffer()
        : cmd_buffer_(VK_NULL_HANDLE)
        , pool_(VK_NULL_HANDLE)
        , is_recording_(false)
    {
    }

    RenderCommandBuffer::RenderCommandBuffer(
        std::shared_ptr<DeviceManager> device,
        VkCommandBuffer                cmd_buffer,
        VkCommandPool                  pool)
        : device_(std::move(device))
        , cmd_buffer_(cmd_buffer)
        , pool_(pool)
        , is_recording_(false)
    {
    }

    RenderCommandBuffer::RenderCommandBuffer(RenderCommandBuffer&& other) noexcept
        : device_(std::move(other.device_))
        , cmd_buffer_(other.cmd_buffer_)
        , pool_(other.pool_)
        , is_recording_(other.is_recording_)
    {
        other.cmd_buffer_   = VK_NULL_HANDLE;
        other.pool_         = VK_NULL_HANDLE;
        other.is_recording_ = false;
    }

    RenderCommandBuffer& RenderCommandBuffer::operator=(RenderCommandBuffer&& other) noexcept
    {
        if (this != &other)
        {
            device_       = std::move(other.device_);
            cmd_buffer_   = other.cmd_buffer_;
            pool_         = other.pool_;
            is_recording_ = other.is_recording_;

            other.cmd_buffer_   = VK_NULL_HANDLE;
            other.pool_         = VK_NULL_HANDLE;
            other.is_recording_ = false;
        }
        return *this;
    }

    void RenderCommandBuffer::begin(VkCommandBufferUsageFlags flags)
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = flags;

        VkResult result = vkBeginCommandBuffer(cmd_buffer_, &begin_info);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to begin command buffer", __FILE__, __LINE__);
        }
        is_recording_ = true;
    }

    void RenderCommandBuffer::end()
    {
        VkResult result = vkEndCommandBuffer(cmd_buffer_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to end command buffer", __FILE__, __LINE__);
        }
        is_recording_ = false;
    }

    void RenderCommandBuffer::reset(VkCommandBufferResetFlags flags)
    {
        VkResult result = vkResetCommandBuffer(cmd_buffer_, flags);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to reset command buffer", __FILE__, __LINE__);
        }
        is_recording_ = false;
    }

    void RenderCommandBuffer::begin_render_pass(
        VkRenderPass                     render_pass,
        VkFramebuffer                    framebuffer,
        const VkRect2D&                  render_area,
        const std::vector<VkClearValue>& clear_values,
        VkSubpassContents                contents)
    {
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass      = render_pass;
        render_pass_info.framebuffer     = framebuffer;
        render_pass_info.renderArea      = render_area;
        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues    = clear_values.data();

        vkCmdBeginRenderPass(cmd_buffer_, &render_pass_info, contents);
    }

    void RenderCommandBuffer::begin_render_pass(
        VkRenderPass        render_pass,
        VkFramebuffer       framebuffer,
        uint32_t            width,
        uint32_t            height,
        const VkClearValue& clear_color,
        VkSubpassContents   contents)
    {
        VkRect2D render_area{};
        render_area.offset = {0, 0};
        render_area.extent = {width, height};

        std::vector<VkClearValue> clear_values = {clear_color};
        begin_render_pass(render_pass, framebuffer, render_area, clear_values, contents);
    }

    void RenderCommandBuffer::end_render_pass()
    {
        vkCmdEndRenderPass(cmd_buffer_);
    }

    void RenderCommandBuffer::bind_pipeline(VkPipeline pipeline)
    {
        vkCmdBindPipeline(cmd_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void RenderCommandBuffer::bind_graphics_pipeline(GraphicsPipeline& pipeline)
    {
        vkCmdBindPipeline(cmd_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle());
    }

    void RenderCommandBuffer::bind_descriptor_sets(
        VkPipelineLayout                    layout,
        uint32_t                            first_set,
        const std::vector<VkDescriptorSet>& descriptor_sets,
        const std::vector<uint32_t>&        dynamic_offsets)
    {
        vkCmdBindDescriptorSets(
                                cmd_buffer_,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout,
                                first_set,
                                static_cast<uint32_t>(descriptor_sets.size()),
                                descriptor_sets.data(),
                                static_cast<uint32_t>(dynamic_offsets.size()),
                                dynamic_offsets.empty() ? nullptr : dynamic_offsets.data());
    }

    void RenderCommandBuffer::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth)
    {
        VkViewport viewport{};
        viewport.x        = x;
        viewport.y        = y;
        viewport.width    = width;
        viewport.height   = height;
        viewport.minDepth = min_depth;
        viewport.maxDepth = max_depth;
        vkCmdSetViewport(cmd_buffer_, 0, 1, &viewport);
    }

    void RenderCommandBuffer::set_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        VkRect2D scissor{};
        scissor.offset = {x, y};
        scissor.extent = {width, height};
        vkCmdSetScissor(cmd_buffer_, 0, 1, &scissor);
    }

    void RenderCommandBuffer::bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset, uint32_t binding)
    {
        vkCmdBindVertexBuffers(cmd_buffer_, binding, 1, &buffer, &offset);
    }

    void RenderCommandBuffer::bind_vertex_buffers(
        uint32_t                         first_binding,
        const std::vector<VkBuffer>&     buffers,
        const std::vector<VkDeviceSize>& offsets)
    {
        vkCmdBindVertexBuffers(cmd_buffer_, first_binding, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets.data());
    }

    void RenderCommandBuffer::bind_index_buffer(VkBuffer buffer, VkIndexType index_type, VkDeviceSize offset)
    {
        vkCmdBindIndexBuffer(cmd_buffer_, buffer, offset, index_type);
    }

    void RenderCommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
    {
        vkCmdDraw(cmd_buffer_, vertex_count, instance_count, first_vertex, first_instance);
    }

    void RenderCommandBuffer::draw_indexed(
        uint32_t index_count,
        uint32_t instance_count,
        uint32_t first_index,
        int32_t  vertex_offset,
        uint32_t first_instance)
    {
        vkCmdDrawIndexed(cmd_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void RenderCommandBuffer::transition_image_layout(
        VkImage                 image,
        VkImageLayout           old_layout,
        VkImageLayout           new_layout,
        VkImageSubresourceRange subresource_range)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout           = old_layout;
        barrier.newLayout           = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;
        barrier.subresourceRange    = subresource_range;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage     = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = 0;
            source_stage          = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destination_stage     = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage     = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        vkCmdPipelineBarrier(
                             cmd_buffer_,
                             source_stage,
                             destination_stage,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);
    }

    void RenderCommandBuffer::transition_image_layout(
        VkImage            image,
        VkImageLayout      old_layout,
        VkImageLayout      new_layout,
        VkImageAspectFlags aspect_mask)
    {
        VkImageSubresourceRange subresource_range{};
        subresource_range.aspectMask     = aspect_mask;
        subresource_range.baseMipLevel   = 0;
        subresource_range.levelCount     = 1;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount     = 1;

        transition_image_layout(image, old_layout, new_layout, subresource_range);
    }

    void RenderCommandBuffer::submit(
        VkQueue                                  queue,
        VkFence                                  fence,
        const std::vector<VkSemaphore>&          wait_semaphores,
        const std::vector<VkPipelineStageFlags>& wait_stages,
        const std::vector<VkSemaphore>&          signal_semaphores)
    {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores    = wait_semaphores.data();
        submit_info.pWaitDstStageMask  = wait_stages.empty() ? nullptr : wait_stages.data();

        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &cmd_buffer_;

        submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
        submit_info.pSignalSemaphores    = signal_semaphores.data();

        VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to submit command buffer", __FILE__, __LINE__);
        }
    }

    // RenderCommandPool implementation
    RenderCommandPool::RenderCommandPool(
        std::shared_ptr<DeviceManager> device,
        uint32_t                       queue_family_index,
        VkCommandPoolCreateFlags       flags)
        : device_(std::move(device))
        , queue_family_index_(queue_family_index)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = queue_family_index;
        pool_info.flags            = flags;

        VkResult result = vkCreateCommandPool(device_->device(), &pool_info, nullptr, &pool_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create command pool", __FILE__, __LINE__);
        }
    }

    RenderCommandPool::~RenderCommandPool()
    {
        if (pool_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyCommandPool(device_->device(), pool_, nullptr);
        }
    }

    RenderCommandPool::RenderCommandPool(RenderCommandPool&& other) noexcept
        : device_(std::move(other.device_))
        , pool_(other.pool_)
        , queue_family_index_(other.queue_family_index_)
    {
        other.pool_ = VK_NULL_HANDLE;
    }

    RenderCommandPool& RenderCommandPool::operator=(RenderCommandPool&& other) noexcept
    {
        if (this != &other)
        {
            if (pool_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyCommandPool(device_->device(), pool_, nullptr);
            }

            device_             = std::move(other.device_);
            pool_               = other.pool_;
            queue_family_index_ = other.queue_family_index_;

            other.pool_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    RenderCommandBuffer RenderCommandPool::allocate(VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = pool_;
        alloc_info.level              = level;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer cmd_buffer;
        VkResult        result = vkAllocateCommandBuffers(device_->device(), &alloc_info, &cmd_buffer);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to allocate command buffer", __FILE__, __LINE__);
        }

        return RenderCommandBuffer(device_, cmd_buffer, pool_);
    }

    std::vector<RenderCommandBuffer> RenderCommandPool::allocate(uint32_t count, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = pool_;
        alloc_info.level              = level;
        alloc_info.commandBufferCount = count;

        std::vector<VkCommandBuffer> cmd_buffers(count);
        VkResult                     result = vkAllocateCommandBuffers(device_->device(), &alloc_info, cmd_buffers.data());
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to allocate command buffers", __FILE__, __LINE__);
        }

        std::vector < RenderCommandBuffer > buffers;
        buffers.reserve(count);
        for (auto cmd_buffer : cmd_buffers)
        {
            buffers.emplace_back(device_, cmd_buffer, pool_);
        }

        return buffers;
    }

    void RenderCommandPool::free(RenderCommandBuffer& buffer)
    {
        if (buffer.handle() != VK_NULL_HANDLE)
        {
            VkCommandBuffer cmd_buffer = buffer.handle();
            vkFreeCommandBuffers(device_->device(), pool_, 1, &cmd_buffer);
            buffer = RenderCommandBuffer();
        }
    }

    void RenderCommandPool::free(std::vector<RenderCommandBuffer>& buffers)
    {
        std::vector<VkCommandBuffer> cmd_buffers;
        cmd_buffers.reserve(buffers.size());
        for (auto& buffer : buffers)
        {
            if (buffer.handle() != VK_NULL_HANDLE)
            {
                cmd_buffers.push_back(buffer.handle());
            }
        }

        if (!cmd_buffers.empty())
        {
            vkFreeCommandBuffers(device_->device(), pool_, static_cast<uint32_t>(cmd_buffers.size()), cmd_buffers.data());
        }

        buffers.clear();
    }

    void RenderCommandPool::reset(VkCommandPoolResetFlags flags)
    {
        VkResult result = vkResetCommandPool(device_->device(), pool_, flags);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to reset command pool", __FILE__, __LINE__);
        }
    }

    // RenderCommandBufferManager implementation
    RenderCommandBufferManager::RenderCommandBufferManager(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    RenderCommandBufferManager::~RenderCommandBufferManager() = default;

    void RenderCommandBufferManager::initialize(uint32_t frame_count, uint32_t queue_family_index)
    {
        pools_.clear();
        buffers_.clear();

        pools_.reserve(frame_count);
        buffers_.reserve(frame_count);

        for (uint32_t i = 0; i < frame_count; ++i)
        {
            auto pool = std::make_unique < RenderCommandPool > (device_, queue_family_index,
                                                                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            buffers_.push_back(pool->allocate());
            pools_.push_back(std::move(pool));
        }
    }

    RenderCommandBuffer& RenderCommandBufferManager::get_current_buffer()
    {
        return buffers_[current_frame_];
    }

    RenderCommandBuffer& RenderCommandBufferManager::get_buffer(uint32_t frame_index)
    {
        return buffers_[frame_index];
    }

    void RenderCommandBufferManager::begin_frame(uint32_t frame_index)
    {
        current_frame_   = frame_index;
        is_frame_active_ = true;

        pools_[current_frame_]->reset();
        buffers_[current_frame_].reset();
        buffers_[current_frame_].begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    void RenderCommandBufferManager::end_frame()
    {
        if (is_frame_active_)
        {
            buffers_[current_frame_].end();
            is_frame_active_ = false;
        }
    }

    void RenderCommandBufferManager::submit(
        VkQueue                                  queue,
        VkFence                                  fence,
        const std::vector<VkSemaphore>&          wait_semaphores,
        const std::vector<VkPipelineStageFlags>& wait_stages,
        const std::vector<VkSemaphore>&          signal_semaphores)
    {
        buffers_[current_frame_].submit(queue, fence, wait_semaphores, wait_stages, signal_semaphores);
    }
} // namespace vulkan_engine::vulkan