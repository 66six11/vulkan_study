#pragma once

#include "vulkan/device/Device.hpp"
#include "platform/windowing/Window.hpp"
#include "rendering/SceneViewport.hpp"

#include <imgui.h>
#include <memory>
#include <functional>
#include <string>

namespace vulkan_engine::editor
{
    // Editor UI Manager - Displays scene viewport and editor panels
    class ImGuiManager
    {
        public:
            struct StatsData
            {
                float       fps              = 0.0f;
                float       frame_time       = 0.0f;
                uint32_t    triangle_count   = 0;
                uint32_t    draw_calls       = 0;
                std::string current_material = "None";
            };

            ImGuiManager();
            ~ImGuiManager();

            // Non-copyable
            ImGuiManager(const ImGuiManager&)            = delete;
            ImGuiManager& operator=(const ImGuiManager&) = delete;

            // Initialize ImGui with Vulkan backend
            void initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<platform::Window>      window,
                VkRenderPass                           render_pass,
                uint32_t                               image_count);

            // Shutdown
            void shutdown();

            // Begin new frame - call at start of frame
            void begin_frame();

            // Draw editor layout with scene viewport
            // Returns viewport panel size for scene rendering
            ImVec2 draw_editor_layout(rendering::SceneViewport* viewport);

            // End frame and prepare draw data
            void end_frame();

            // Render ImGui to command buffer
            void render(VkCommandBuffer command_buffer);

            // Check if initialized
            bool is_initialized() const { return initialized_; }

            // Check if viewport is focused (for camera input)
            bool is_viewport_focused() const { return viewport_focused_; }
            bool is_viewport_hovered() const { return viewport_hovered_; }

            // Check if mouse is over viewport content (not title bar)
            bool is_viewport_content_hovered() const { return viewport_content_hovered_; }

            // Update stats data
            void update_stats(const StatsData& stats) { stats_data_ = stats; }

        private:
            void create_descriptor_pool(uint32_t image_count);
            void setup_platform_bindings();
            void draw_menu_bar();
            void draw_stats_panel();
            void draw_material_panel();
            void draw_scene_hierarchy();

            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<platform::Window>      window_;

            VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

            // State
            bool      initialized_              = false;
            bool      viewport_focused_         = false;
            bool      viewport_hovered_         = false;
            bool      viewport_content_hovered_ = false;
            StatsData stats_data_;

            // Panel visibility
            bool show_stats_panel_     = true;
            bool show_material_panel_  = true;
            bool show_scene_hierarchy_ = true;
            bool show_demo_window_     = false;
    };
} // namespace vulkan_engine::rendering