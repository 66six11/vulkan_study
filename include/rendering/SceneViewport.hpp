#pragma once

#include "vulkan/device/Device.hpp"
#include "vulkan/resources/Image.hpp"

#include <vulkan/vulkan.h>
#include <memory>

// Forward declare ImTextureID to avoid including imgui.h in header
// ImTextureID is void* in ImGui
using ImTextureID = void*;

namespace vulkan_engine::rendering
{
    // Off-screen render target for 3D scene
    // Renders to texture instead of swapchain for display in ImGui panel
    class SceneViewport
    {
        public:
            struct CreateInfo
            {
                uint32_t              width        = 1280;
                uint32_t              height       = 720;
                VkFormat              color_format = VK_FORMAT_B8G8R8A8_UNORM;
                VkFormat              depth_format = VK_FORMAT_D32_SFLOAT;
                VkSampleCountFlagBits samples      = VK_SAMPLE_COUNT_1_BIT;
            };

            SceneViewport();
            ~SceneViewport();

            // Non-copyable
            SceneViewport(const SceneViewport&)            = delete;
            SceneViewport& operator=(const SceneViewport&) = delete;

            // Movable
            SceneViewport(SceneViewport&& other) noexcept;
            SceneViewport& operator=(SceneViewport&& other) noexcept;

            // Initialize viewport with given size
            void initialize(std::shared_ptr<vulkan::DeviceManager> device, const CreateInfo& info);

            // Request resize (delayed until next frame)
            void request_resize(uint32_t width, uint32_t height);

            // Apply pending resize (call at start of frame)
            void apply_pending_resize();

            // Check if resize is pending
            bool is_resize_pending() const { return resize_pending_; }

            // Resize viewport (immediate - use with caution)
            void resize(uint32_t width, uint32_t height);

            // Cleanup resources
            void cleanup();

            // Getters
            VkImageView color_image_view() const { return color_image_view_; }
            VkImageView depth_image_view() const { return depth_image_view_; }
            VkImage     color_image() const { return color_image_; }
            VkImage     depth_image() const { return depth_image_; }
            VkExtent2D  extent() const { return {width_, height_}; }                         // Render target size
            VkExtent2D  display_extent() const { return {display_width_, display_height_}; } // Display size
            uint32_t    width() const { return width_; }
            uint32_t    height() const { return height_; }

            // ImGui texture ID (for displaying in UI)
            ImTextureID imgui_texture_id() const;

            // Begin render pass for scene rendering
            void begin_render_pass(VkCommandBuffer command_buffer);
            void end_render_pass(VkCommandBuffer command_buffer);

            // Get render pass handle (for pipeline creation)
            VkRenderPass render_pass() const { return render_pass_; }

            // Get framebuffer handle (for render graph)
            VkFramebuffer framebuffer() const { return framebuffer_; }

        private:
            void create_images();
            void create_framebuffer();
            void create_render_pass();
            void transition_image_layout(); // Initial layout transition

            std::shared_ptr<vulkan::DeviceManager> device_;

            // Configuration
            uint32_t              width_          = 0; // Render target width (may be larger)
            uint32_t              height_         = 0; // Render target height (may be larger)
            uint32_t              display_width_  = 0; // Actual display width
            uint32_t              display_height_ = 0; // Actual display height
            VkFormat              color_format_   = VK_FORMAT_UNDEFINED;
            VkFormat              depth_format_   = VK_FORMAT_UNDEFINED;
            VkSampleCountFlagBits samples_        = VK_SAMPLE_COUNT_1_BIT;

            // Color target
            VkImage        color_image_      = VK_NULL_HANDLE;
            VkDeviceMemory color_memory_     = VK_NULL_HANDLE;
            VkImageView    color_image_view_ = VK_NULL_HANDLE;

            // Depth target
            VkImage        depth_image_      = VK_NULL_HANDLE;
            VkDeviceMemory depth_memory_     = VK_NULL_HANDLE;
            VkImageView    depth_image_view_ = VK_NULL_HANDLE;

            // Render pass & framebuffer
            VkRenderPass  render_pass_ = VK_NULL_HANDLE;
            VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

            // ImGui descriptor set and sampler for the color texture
            VkDescriptorSet imgui_descriptor_set_ = VK_NULL_HANDLE;
            VkSampler       imgui_sampler_        = VK_NULL_HANDLE;

            // Delayed resize to avoid resizing during rendering
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
            bool     resize_pending_ = false;
    };
} // namespace vulkan_engine::rendering