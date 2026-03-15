#pragma once

#include "editor/ImGuiManager.hpp"
#include "rendering/render_graph/RenderGraph.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "platform/windowing/Window.hpp"

#include <memory>

namespace vulkan_engine::rendering
{
    class RenderTarget;
    class Viewport;
}

namespace vulkan_engine::editor
{
    // Main Editor class - Integrates viewport, UI, and scene rendering
    class Editor
    {
        public:
            Editor();
            ~Editor();

            // Non-copyable
            Editor(const Editor&)            = delete;
            Editor& operator=(const Editor&) = delete;

            // Initialize editor with window and device
            void initialize(
                std::shared_ptr<platform::Window>        window,
                std::shared_ptr<vulkan::DeviceManager>   device,
                std::shared_ptr<vulkan::SwapChain>       swap_chain,
                std::shared_ptr<rendering::RenderTarget> render_target = nullptr,
                std::shared_ptr<rendering::Viewport>     viewport      = nullptr);

            // Shutdown and cleanup
            void shutdown();

            // Begin editor frame - call at start of application frame
            void begin_frame();

            // Render scene to viewport - call during rendering
            // Returns command buffer for submission
            VkCommandBuffer render_scene(
                std::shared_ptr<rendering::RenderGraph> render_graph);

            // End editor frame and render UI - call after scene rendering
            void end_frame(uint32_t image_index);

            // Render ImGui to command buffer - call during swap chain render pass
            void render_to_command_buffer(VkCommandBuffer command_buffer);

            // Get render target for rendering
            std::shared_ptr<rendering::RenderTarget> render_target() const { return render_target_; }

            // Get viewport for display logic
            std::shared_ptr<rendering::Viewport> viewport() const { return viewport_; }

            // Check if viewport is focused/hovered (for camera controls)
            bool is_viewport_focused() const;
            bool is_viewport_hovered() const;

            // Check if mouse is over viewport content (not title bar)
            bool is_viewport_content_hovered() const;

            // Update stats display
            void update_stats(const ImGuiManager::StatsData& stats);


            // Check if initialized
            bool is_initialized() const { return initialized_; }

            // Recreate render pass after window resize
            void recreate_render_pass(VkRenderPass render_pass, uint32_t image_count);

            // Set callback for viewport resize events (called when viewport size changes)
            using ViewportResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
            void set_viewport_resize_callback(ViewportResizeCallback callback) { viewport_resize_callback_ = callback; }

        private:
            void create_command_pool();
            void allocate_command_buffers(uint32_t count);
            void create_sync_objects();

            std::shared_ptr<platform::Window>      window_;
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<vulkan::SwapChain>     swap_chain_;

            std::unique_ptr<ImGuiManager>            imgui_manager_;
            std::shared_ptr<rendering::RenderTarget> render_target_;
            std::shared_ptr<rendering::Viewport>     viewport_;

            // Command buffers for viewport rendering
            VkCommandPool                command_pool_ = VK_NULL_HANDLE;
            std::vector<VkCommandBuffer> command_buffers_;

            // ImGui uses swap chain images
            uint32_t current_image_index_ = 0;

            // Callback for viewport resize events
            ViewportResizeCallback viewport_resize_callback_;

            bool initialized_ = false;
    };
} // namespace vulkan_engine::editor