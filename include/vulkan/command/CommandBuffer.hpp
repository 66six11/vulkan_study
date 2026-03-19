#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>

namespace vulkan_engine::vulkan
{
    // Forward declarations
    class Framebuffer;
    class GraphicsPipeline;

    // Command buffer wrapper for easy recording
    class RenderCommandBuffer
    {
        public:
            RenderCommandBuffer();
            RenderCommandBuffer(
                std::shared_ptr<DeviceManager> device,
                VkCommandBuffer                cmd_buffer,
                VkCommandPool                  pool);
            ~RenderCommandBuffer() = default;

            // Non-copyable
            RenderCommandBuffer(const RenderCommandBuffer&)            = delete;
            RenderCommandBuffer& operator=(const RenderCommandBuffer&) = delete;

            // Movable
            RenderCommandBuffer(RenderCommandBuffer&& other) noexcept;
            RenderCommandBuffer& operator=(RenderCommandBuffer&& other) noexcept;

            // Recording
            void begin(VkCommandBufferUsageFlags flags = 0);
            void end();
            void reset(VkCommandBufferResetFlags flags = 0);

            // Render pass
            void begin_render_pass(
                VkRenderPass                     render_pass,
                VkFramebuffer                    framebuffer,
                const VkRect2D&                  render_area,
                const std::vector<VkClearValue>& clear_values,
                VkSubpassContents                contents = VK_SUBPASS_CONTENTS_INLINE);

            void begin_render_pass(
                VkRenderPass        render_pass,
                VkFramebuffer       framebuffer,
                uint32_t            width,
                uint32_t            height,
                const VkClearValue& clear_color,
                VkSubpassContents   contents = VK_SUBPASS_CONTENTS_INLINE);

            void end_render_pass();

            // Dynamic Rendering (Vulkan 1.3+)
            void begin_dynamic_rendering(
                VkImageView         color_view,
                VkImageView         depth_view,
                uint32_t            width,
                uint32_t            height,
                const VkClearValue* color_clear = nullptr,
                const VkClearValue* depth_clear = nullptr);

            void end_dynamic_rendering();

            // Pipeline
            void bind_pipeline(VkPipeline pipeline);
            void bind_graphics_pipeline(GraphicsPipeline& pipeline);

            // Viewport and scissor
            void set_viewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);
            void set_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

            // Vertex and index buffers
            void bind_vertex_buffer(VkBuffer buffer, VkDeviceSize offset = 0, uint32_t binding = 0);
            void bind_vertex_buffers(uint32_t first_binding, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets);
            void bind_index_buffer(VkBuffer buffer, VkIndexType index_type, VkDeviceSize offset = 0);

            // Descriptor sets
            void bind_descriptor_sets(
                VkPipelineLayout                    layout,
                uint32_t                            first_set,
                const std::vector<VkDescriptorSet>& descriptor_sets,
                const std::vector<uint32_t>&        dynamic_offsets = {});

            // Push constants
            void push_constants(
                VkPipelineLayout   layout,
                VkShaderStageFlags stage_flags,
                uint32_t           offset,
                uint32_t           size,
                const void*        values);

            template <typename T> void push_constants(VkPipelineLayout layout, VkShaderStageFlags stage_flags, const T& value)
            {
                push_constants(layout, stage_flags, 0, sizeof(T), &value);
            }

            // Draw commands
            void draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0);
            void draw_indexed(
                uint32_t index_count,
                uint32_t instance_count = 1,
                uint32_t first_index    = 0,
                int32_t  vertex_offset  = 0,
                uint32_t first_instance = 0);

            // Image transitions
            void transition_image_layout(
                VkImage                 image,
                VkImageLayout           old_layout,
                VkImageLayout           new_layout,
                VkImageSubresourceRange subresource_range);

            void transition_image_layout(
                VkImage            image,
                VkImageLayout      old_layout,
                VkImageLayout      new_layout,
                VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT);

            // Copy commands
            void copy_buffer_to_image(
                VkBuffer                              src_buffer,
                VkImage                               dst_image,
                VkImageLayout                         dst_image_layout,
                const std::vector<VkBufferImageCopy>& regions);

            // Accessors
            VkCommandBuffer handle() const { return cmd_buffer_; }
            bool            valid() const { return cmd_buffer_ != VK_NULL_HANDLE; }
            bool            is_recording() const { return is_recording_; }

            // Submit helpers
            void submit(
                VkQueue                                  queue,
                VkFence                                  fence             = VK_NULL_HANDLE,
                const std::vector<VkSemaphore>&          wait_semaphores   = {},
                const std::vector<VkPipelineStageFlags>& wait_stages       = {},
                const std::vector<VkSemaphore>&          signal_semaphores = {});

        private:
            std::shared_ptr<DeviceManager> device_;
            VkCommandBuffer                cmd_buffer_   = VK_NULL_HANDLE;
            VkCommandPool                  pool_         = VK_NULL_HANDLE;
            bool                           is_recording_ = false;
    };

    // Command pool manager
    class RenderCommandPool
    {
        public:
            RenderCommandPool(
                std::shared_ptr<DeviceManager> device,
                uint32_t                       queue_family_index,
                VkCommandPoolCreateFlags       flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            ~RenderCommandPool();

            // Non-copyable
            RenderCommandPool(const RenderCommandPool&)            = delete;
            RenderCommandPool& operator=(const RenderCommandPool&) = delete;

            // Movable
            RenderCommandPool(RenderCommandPool&& other) noexcept;
            RenderCommandPool& operator=(RenderCommandPool&& other) noexcept;

            // Allocation
            RenderCommandBuffer              allocate(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            std::vector<RenderCommandBuffer> allocate(uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

            // Free individual buffers (only for VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
            void free(RenderCommandBuffer& buffer);
            void free(std::vector<RenderCommandBuffer>& buffers);

            // Reset entire pool (faster than individual resets)
            void reset(VkCommandPoolResetFlags flags = 0);

            VkCommandPool handle() const { return pool_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkCommandPool                  pool_               = VK_NULL_HANDLE;
            uint32_t                       queue_family_index_ = 0;
    };

    // Command buffer manager for frame-based rendering
    class RenderCommandBufferManager
    {
        public:
            explicit RenderCommandBufferManager(std::shared_ptr<DeviceManager> device);
            ~RenderCommandBufferManager();

            // Initialize with frame count (for double/triple buffering)
            void initialize(uint32_t frame_count, uint32_t queue_family_index);

            // Get command buffer for current frame
            RenderCommandBuffer& get_current_buffer();
            RenderCommandBuffer& get_buffer(uint32_t frame_index);

            // Begin/end frame
            void begin_frame(uint32_t frame_index);
            void end_frame();

            // Submit current frame's command buffer
            void submit(
                VkQueue                                  queue,
                VkFence                                  fence             = VK_NULL_HANDLE,
                const std::vector<VkSemaphore>&          wait_semaphores   = {},
                const std::vector<VkPipelineStageFlags>& wait_stages       = {},
                const std::vector<VkSemaphore>&          signal_semaphores = {});

            uint32_t frame_count() const { return static_cast<uint32_t>(pools_.size()); }

        private:
            std::shared_ptr<DeviceManager>                  device_;
            std::vector<std::unique_ptr<RenderCommandPool>> pools_;
            std::vector<RenderCommandBuffer>                buffers_;
            uint32_t                                        current_frame_   = 0;
            bool                                            is_frame_active_ = false;
    };
} // namespace vulkan_engine::vulkan