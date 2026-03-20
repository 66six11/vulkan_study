#pragma once

#include "rendering/render_graph/RenderGraphTypes.hpp"
#include "vulkan/resources/Image.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/command/CommandBuffer.hpp"

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace vulkan_engine::rendering
{
    // Resource state for barrier tracking
    struct ResourceState
    {
        VkPipelineStageFlags stage        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkAccessFlags        access       = 0;
        VkImageLayout        layout       = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t             queue_family = VK_QUEUE_FAMILY_IGNORED;
    };

    // Image resource info
    struct ImageResourceInfo
    {
        ImageHandle                    handle;
        ResourceDesc                   desc;
        ResourceState                  state;
        std::unique_ptr<vulkan::Image> image;
        VkImageView                    view        = VK_NULL_HANDLE;
        bool                           is_external = false;
    };

    // Buffer resource info
    struct BufferResourceInfo
    {
        BufferHandle                    handle;
        ResourceDesc                    desc;
        ResourceState                   state;
        std::unique_ptr<vulkan::Buffer> buffer;
        bool                            is_external = false;
    };

    // Resource pool for managing render graph resources
    class RenderGraphResourcePool
    {
        public:
            explicit RenderGraphResourcePool(std::shared_ptr<vulkan::DeviceManager> device);
            ~RenderGraphResourcePool();

            // Create or acquire an image resource
            ImageResourceInfo* acquire_image(const ResourceDesc& desc, ImageHandle handle);

            // Create or acquire a buffer resource
            BufferResourceInfo* acquire_buffer(const ResourceDesc& desc, BufferHandle handle);

            // Import external image (e.g., swap chain image)
            ImageResourceInfo* import_image(
                ImageHandle handle,
                VkImage     image,
                VkImageView view,
                VkFormat    format,
                uint32_t    width,
                uint32_t    height);

            // Get resource by handle
            ImageResourceInfo*  get_image(ImageHandle handle);
            BufferResourceInfo* get_buffer(BufferHandle handle);

            // Release all resources
            void reset();

            // Generate barriers for resource transition
            void generate_barriers(ImageHandle handle, ResourceState new_state, struct BarrierBatch& batch);
            void generate_barriers(BufferHandle handle, ResourceState new_state, struct BarrierBatch& batch);

            // Submit barriers to command buffer
            static void submit_barriers(vulkan::RenderCommandBuffer& cmd, const BarrierBatch& batch);

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;

            std::unordered_map<uint32_t, std::unique_ptr<ImageResourceInfo>>  images_;
            std::unordered_map<uint32_t, std::unique_ptr<BufferResourceInfo>> buffers_;

            // Resource creation helpers
            std::unique_ptr<vulkan::Image>  create_image(const ResourceDesc& desc);
            std::unique_ptr<vulkan::Buffer> create_buffer(const ResourceDesc& desc);
    };

    // Resource usage tracking per pass
    struct ResourceUsage
    {
        enum class AccessType
        {
            Read,
            Write,
            ReadWrite
        };

        AccessType    access;
        ResourceState state;
    };

    // Render pass resource requirements
    struct PassResources
    {
        std::unordered_map<uint32_t, ResourceUsage> image_usage;
        std::unordered_map<uint32_t, ResourceUsage> buffer_usage;
    };

    // Automatic barrier manager
    class BarrierManager
    {
        public:
            void register_pass(uint32_t pass_index, const PassResources& resources);

            void compile(RenderGraphResourcePool& pool);

            // Get barriers to insert before a pass
            BarrierBatch get_barriers_for_pass(uint32_t pass_index) const;

            void clear();

        private:
            struct ResourceTransition
            {
                uint32_t      resource_id;
                ResourceState from;
                ResourceState to;
                bool          is_image;
            };

            std::unordered_map<uint32_t, PassResources>                   pass_resources_;
            std::unordered_map<uint32_t, std::vector<ResourceTransition>> pass_transitions_;
    };
} // namespace vulkan_engine::rendering