#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/render_graph/RenderGraphResource.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "core/utils/Logger.hpp"
#include <algorithm>
#include <stdexcept>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // RenderGraph Implementation
    // ============================================================================

    RenderGraph::RenderGraph() = default;

    RenderGraph::~RenderGraph()
    {
        // Ensure resources are cleaned up before device is destroyed
        reset();
    }

    void RenderGraph::initialize(std::shared_ptr<vulkan::DeviceManager> device)
    {
        device_          = std::move(device);
        resource_pool_   = std::make_unique<RenderGraphResourcePool>(device_);
        barrier_manager_ = std::make_unique<BarrierManager>();
    }

    void RenderGraph::reset()
    {
        compiled_ = false;
        execution_order_.clear();
        pass_barriers_.clear();
        if (resource_pool_)
        {
            resource_pool_->reset();
        }
        if (barrier_manager_)
        {
            barrier_manager_->clear();
        }
        next_resource_id_ = 1;
    }

    ImageHandle RenderGraph::create_image(const ResourceDesc& desc)
    {
        ImageHandle handle(next_resource_id_++, 1);
        if (resource_pool_)
        {
            resource_pool_->acquire_image(desc, handle);
        }
        return handle;
    }

    BufferHandle RenderGraph::create_buffer(const ResourceDesc& desc)
    {
        BufferHandle handle(next_resource_id_++, 1);
        if (resource_pool_)
        {
            resource_pool_->acquire_buffer(desc, handle);
        }
        return handle;
    }

    ImageHandle RenderGraph::import_image(
        VkImage            image,
        VkImageView        view,
        VkFormat           format,
        uint32_t           width,
        uint32_t           height,
        const std::string& name)
    {
        (void)name;
        ImageHandle handle(next_resource_id_++, 1);
        if (resource_pool_)
        {
            resource_pool_->import_image(handle, image, view, format, width, height);
        }
        return handle;
    }

    void RenderGraph::build_execution_order()
    {
        execution_order_.clear();
        for (const auto& node : builder_.nodes())
        {
            execution_order_.push_back(node.get());
        }
    }

    void RenderGraph::generate_barriers()
    {
        if (!resource_pool_ || !barrier_manager_)
        {
            return;
        }

        pass_barriers_.resize(execution_order_.size());

        for (size_t i = 0; i < execution_order_.size(); ++i)
        {
            auto* node = execution_order_[i];

            // Get barriers from barrier manager
            pass_barriers_[i] = barrier_manager_->get_barriers_for_pass(static_cast<uint32_t>(i));

            // Also check for image transitions needed by this pass
            if (auto* pass_base = dynamic_cast<RenderPassBase*>(node))
            {
                // Get image outputs - they need to be transitioned to attachment optimal
                for (const auto& img_handle : pass_base->get_image_outputs())
                {
                    ResourceState new_state{};
                    new_state.stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    new_state.access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    new_state.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    resource_pool_->generate_barriers(img_handle, new_state, pass_barriers_[i]);
                }

                // Get image inputs - they need to be transitioned to shader read
                for (const auto& img_handle : pass_base->get_image_inputs())
                {
                    ResourceState new_state{};
                    new_state.stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    new_state.access = VK_ACCESS_SHADER_READ_BIT;
                    new_state.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    resource_pool_->generate_barriers(img_handle, new_state, pass_barriers_[i]);
                }
            }
        }
    }

    // RenderGraphBuilder implementation
    BufferHandle RenderGraphBuilder::create_buffer(const ResourceDesc& desc)
    {
        static uint32_t next_id = 1;
        BufferHandle    handle(next_id++, 1);
        resources_[desc.name] = handle;
        return handle;
    }

    ImageHandle RenderGraphBuilder::create_image(const ResourceDesc& desc)
    {
        static uint32_t next_id = 1;
        ImageHandle     handle(next_id++, 1);
        resources_[desc.name] = handle;
        return handle;
    }

    TextureHandle RenderGraphBuilder::create_texture(const ResourceDesc& desc)
    {
        static uint32_t next_id = 1;
        TextureHandle   handle(next_id++, 1);
        resources_[desc.name] = handle;
        return handle;
    }

    void RenderGraphBuilder::read(BufferHandle /*buffer*/)
    {
    }

    void RenderGraphBuilder::write(BufferHandle /*buffer*/)
    {
    }

    void RenderGraphBuilder::read(ImageHandle /*image*/)
    {
    }

    void RenderGraphBuilder::write(ImageHandle /*image*/)
    {
    }

    // RenderGraph implementation
    void RenderGraph::compile()
    {
        if (!resource_pool_)
        {
            logger::error("RenderGraph not initialized - call initialize() first");
            return;
        }

        logger::info("Compiling RenderGraph...");

        // Build execution order based on dependencies
        build_execution_order();

        // Generate barriers for resource transitions
        generate_barriers();

        compiled_ = true;
        logger::info("RenderGraph compiled successfully with " + std::to_string(execution_order_.size()) + " passes");
    }

    void RenderGraph::execute()
    {
        if (!compiled_)
        {
            compile();
        }

        logger::warn("RenderGraph::execute() without command buffer not implemented");
    }

    void RenderGraph::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx)
    {
        if (!compiled_)
        {
            compile();
        }

        if (!resource_pool_)
        {
            logger::error("RenderGraph resource pool not available");
            return;
        }

        // Execute each node in order with barriers
        for (size_t i = 0; i < execution_order_.size(); ++i)
        {
            auto* node = execution_order_[i];

            // Submit barriers before this pass
            if (i < pass_barriers_.size())
            {
                RenderGraphResourcePool::submit_barriers(cmd, pass_barriers_[i]);
            }

            // Execute the pass
            if (auto* pass_base = dynamic_cast<RenderPassBase*>(node))
            {
                pass_base->execute(cmd, ctx);
            }
            else
            {
                // Fallback to basic execute - would need a different interface
                logger::warn("Pass " + std::string(node->name()) + " does not support RenderContext");
            }
        }
    }
} // namespace vulkan_engine::rendering