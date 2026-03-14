#include "rendering/render_graph/RenderGraphResource.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <algorithm>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // RenderGraphResourcePool
    // ============================================================================

    RenderGraphResourcePool::RenderGraphResourcePool(std::shared_ptr<vulkan::DeviceManager> device)
        : device_(std::move(device))
    {
    }

    RenderGraphResourcePool::~RenderGraphResourcePool() = default;

    ImageResourceInfo* RenderGraphResourcePool::acquire_image(const ResourceDesc& desc, ImageHandle handle)
    {
        uint32_t id = handle.id();

        auto it = images_.find(id);
        if (it != images_.end())
        {
            // Return existing resource
            return it->second.get();
        }

        // Create new image
        auto info         = std::make_unique<ImageResourceInfo>();
        info->handle      = handle;
        info->desc        = desc;
        info->state       = {}; // Default state
        info->is_external = false;
        info->image       = create_image(desc);

        // Create image view
        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = info->image->handle();
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = info->image->format();
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        VkResult result = vkCreateImageView(device_->device(), &view_info, nullptr, &info->view);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to create image view", __FILE__, __LINE__);
        }

        ImageResourceInfo* ptr = info.get();
        images_[id]            = std::move(info);
        return ptr;
    }

    BufferResourceInfo* RenderGraphResourcePool::acquire_buffer(const ResourceDesc& desc, BufferHandle handle)
    {
        uint32_t id = handle.id();

        auto it = buffers_.find(id);
        if (it != buffers_.end())
        {
            return it->second.get();
        }

        auto info         = std::make_unique<BufferResourceInfo>();
        info->handle      = handle;
        info->desc        = desc;
        info->state       = {};
        info->is_external = false;
        info->buffer      = create_buffer(desc);

        BufferResourceInfo* ptr = info.get();
        buffers_[id]            = std::move(info);
        return ptr;
    }

    ImageResourceInfo* RenderGraphResourcePool::import_image(
        ImageHandle handle,
        VkImage /*image*/,
        VkImageView view,
        VkFormat /*format*/,
        uint32_t width,
        uint32_t height)
    {
        uint32_t id = handle.id();

        auto info              = std::make_unique<ImageResourceInfo>();
        info->handle           = handle;
        info->desc.type        = ResourceDesc::Type::Image;
        info->desc.width       = width;
        info->desc.height      = height;
        info->desc.format      = ResourceDesc::Format::R8G8B8A8_UNORM; // Simplified
        info->desc.is_external = true;
        info->is_external      = true;
        info->view             = view;
        // Note: We don't own the image, so we don't create a wrapper
        info->state.layout = VK_IMAGE_LAYOUT_UNDEFINED;

        ImageResourceInfo* ptr = info.get();
        images_[id]            = std::move(info);
        return ptr;
    }

    ImageResourceInfo* RenderGraphResourcePool::get_image(ImageHandle handle)
    {
        auto it = images_.find(handle.id());
        if (it != images_.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    BufferResourceInfo* RenderGraphResourcePool::get_buffer(BufferHandle handle)
    {
        auto it = buffers_.find(handle.id());
        if (it != buffers_.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    void RenderGraphResourcePool::reset()
    {
        // Clean up image views
        for (auto& [id, info] : images_)
        {
            if (info->view != VK_NULL_HANDLE && !info->is_external)
            {
                vkDestroyImageView(device_->device(), info->view, nullptr);
            }
        }
        images_.clear();
        buffers_.clear();
    }

    std::unique_ptr<vulkan::Image> RenderGraphResourcePool::create_image(const ResourceDesc& desc)
    {
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        switch (desc.format)
        {
            case ResourceDesc::Format::R16G16B16A16_SFLOAT:
                format = VK_FORMAT_R16G16B16A16_SFLOAT;
                break;
            case ResourceDesc::Format::D32_SFLOAT:
                format = VK_FORMAT_D32_SFLOAT;
                break;
            default:
                break;
        }

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (desc.format == ResourceDesc::Format::D32_SFLOAT)
        {
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        return std::make_unique<vulkan::Image>(
                                               device_,
                                               desc.width,
                                               desc.height,
                                               format,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               usage,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    std::unique_ptr<vulkan::Buffer> RenderGraphResourcePool::create_buffer(const ResourceDesc& desc)
    {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (desc.type == ResourceDesc::Type::Buffer)
        {
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        return std::make_unique<vulkan::Buffer>(
                                                device_,
                                                desc.size,
                                                usage,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    void RenderGraphResourcePool::generate_barriers(ImageHandle handle, ResourceState new_state, BarrierBatch& batch)
    {
        ImageResourceInfo* info = get_image(handle);
        if (!info || info->is_external)
        {
            return;
        }

        // Check if layout transition is needed
        if (info->state.layout != new_state.layout ||
            info->state.access != new_state.access ||
            info->state.stage != new_state.stage)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                   = info->state.layout;
            barrier.newLayout                   = new_state.layout;
            barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                       = info->image->handle();
            barrier.subresourceRange.aspectMask = (new_state.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                                                      ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                      : VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = info->state.access;
            barrier.dstAccessMask                   = new_state.access;

            batch.image_barriers.push_back(barrier);
            batch.src_stage |= info->state.stage;
            batch.dst_stage |= new_state.stage;

            // Update stored state
            info->state = new_state;
        }
    }

    void RenderGraphResourcePool::generate_barriers(BufferHandle handle, ResourceState new_state, BarrierBatch& batch)
    {
        BufferResourceInfo* info = get_buffer(handle);
        if (!info)
        {
            return;
        }

        if (info->state.access != new_state.access || info->state.stage != new_state.stage)
        {
            VkBufferMemoryBarrier barrier{};
            barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask       = info->state.access;
            barrier.dstAccessMask       = new_state.access;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer              = info->buffer->handle();
            barrier.offset              = 0;
            barrier.size                = VK_WHOLE_SIZE;

            batch.buffer_barriers.push_back(barrier);
            batch.src_stage |= info->state.stage;
            batch.dst_stage |= new_state.stage;

            info->state = new_state;
        }
    }

    void RenderGraphResourcePool::submit_barriers(vulkan::RenderCommandBuffer& cmd, const BarrierBatch& batch)
    {
        if (batch.empty())
        {
            return;
        }

        vkCmdPipelineBarrier(
                             cmd.handle(),
                             batch.src_stage,
                             batch.dst_stage,
                             0,
                             0,
                             nullptr,
                             static_cast<uint32_t>(batch.buffer_barriers.size()),
                             batch.buffer_barriers.empty() ? nullptr : batch.buffer_barriers.data(),
                             static_cast<uint32_t>(batch.image_barriers.size()),
                             batch.image_barriers.empty() ? nullptr : batch.image_barriers.data());
    }

    // ============================================================================
    // BarrierManager Implementation
    // ============================================================================

    void BarrierManager::register_pass(uint32_t pass_index, const PassResources& resources)
    {
        pass_resources_[pass_index] = resources;
    }

    void BarrierManager::compile(RenderGraphResourcePool& /*pool*/)
    {
        // Analyze dependencies between passes
        // For now, we do simple sequential analysis
        pass_transitions_.clear();

        for (uint32_t i = 0; i < static_cast<uint32_t>(pass_resources_.size()); ++i)
        {
            auto& resources = pass_resources_[i];

            // Check image transitions
            for (const auto& [img_id, usage] : resources.image_usage)
            {
                ResourceTransition transition{};
                transition.resource_id = img_id;
                transition.to          = usage.state;
                transition.is_image    = true;

                // Find previous state
                if (i > 0)
                {
                    auto& prev_resources = pass_resources_[i - 1];
                    auto  it             = prev_resources.image_usage.find(img_id);
                    if (it != prev_resources.image_usage.end())
                    {
                        transition.from = it->second.state;
                    }
                }

                pass_transitions_[i].push_back(transition);
            }
        }
    }

    BarrierBatch BarrierManager::get_barriers_for_pass(uint32_t pass_index) const
    {
        BarrierBatch batch;

        auto it = pass_transitions_.find(pass_index);
        if (it == pass_transitions_.end())
        {
            return batch;
        }

        // Build barriers from transitions
        for (const auto& transition : it->second)
        {
            if (transition.is_image)
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout                       = transition.from.layout;
                barrier.newLayout                       = transition.to.layout;
                barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.srcAccessMask                   = transition.from.access;
                barrier.dstAccessMask                   = transition.to.access;
                barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel   = 0;
                barrier.subresourceRange.levelCount     = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount     = 1;

                batch.image_barriers.push_back(barrier);
                batch.src_stage |= transition.from.stage;
                batch.dst_stage |= transition.to.stage;
            }
            else
            {
                VkBufferMemoryBarrier barrier{};
                barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask       = transition.from.access;
                barrier.dstAccessMask       = transition.to.access;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.offset              = 0;
                barrier.size                = VK_WHOLE_SIZE;

                batch.buffer_barriers.push_back(barrier);
                batch.src_stage |= transition.from.stage;
                batch.dst_stage |= transition.to.stage;
            }
        }

        return batch;
    }

    void BarrierManager::clear()
    {
        pass_resources_.clear();
        pass_transitions_.clear();
    }
} // namespace vulkan_engine::rendering