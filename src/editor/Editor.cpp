#include "editor/Editor.hpp"
#include "core/utils/Logger.hpp"
#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/SceneViewport.hpp"
#include "vulkan/utils/VulkanError.hpp"

#include <imgui_impl_vulkan.h>

namespace vulkan_engine::editor
{
    Editor::Editor() = default;

    Editor::~Editor()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    void Editor::initialize(
        std::shared_ptr<platform::Window>      window,
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<vulkan::SwapChain>     swap_chain)
    {
        window_     = window;
        device_     = device;
        swap_chain_ = swap_chain;

        // Get swap chain images for ImGui
        uint32_t image_count = swap_chain_->image_count();

        // Create ImGui manager
        imgui_manager_ = std::make_unique<ImGuiManager>();

        // Create viewport with default size
        viewport_ = std::make_unique<rendering::SceneViewport>();
        rendering::SceneViewport::CreateInfo viewport_info{};
        viewport_info.width  = 1280;
        viewport_info.height = 720;
        viewport_->initialize(device_, viewport_info);

        // Initialize ImGui with swap chain render pass
        // Note: ImGui renders to swap chain, not viewport
        VkRenderPass render_pass = swap_chain_->default_render_pass();
        if (render_pass == VK_NULL_HANDLE)
        {
            logger::error("SwapChain default_render_pass is null!");
        }
        else
        {
            logger::info("SwapChain default_render_pass is valid");
        }
        imgui_manager_->initialize(device_, window_, render_pass, image_count);

        // Create command pool for viewport rendering
        create_command_pool();
        allocate_command_buffers(image_count);

        initialized_ = true;
        logger::info("Editor initialized");
    }

    void Editor::shutdown()
    {
        if (!initialized_ || !device_)
        {
            return;
        }

        vkDeviceWaitIdle(device_->device());

        imgui_manager_->shutdown();
        viewport_->cleanup();

        if (command_pool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device_->device(), command_pool_, nullptr);
            command_pool_ = VK_NULL_HANDLE;
        }

        initialized_ = false;
        logger::info("Editor shutdown");
    }

    void Editor::begin_frame()
    {
        if (!initialized_)
        {
            return;
        }

        imgui_manager_->begin_frame();
    }

    VkCommandBuffer Editor::render_scene(
        std::shared_ptr<rendering::RenderGraph> render_graph)
    {
        (void)render_graph; // TODO: Use render graph for scene rendering
        if (!initialized_)
        {
            return VK_NULL_HANDLE;
        }

        VkCommandBuffer cmd = command_buffers_[current_image_index_];

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin_info);

        // Begin viewport render pass
        viewport_->begin_render_pass(cmd);

        // Execute render graph
        // Note: RenderGraph needs to be modified to support custom render pass
        // For now, we just clear the viewport
        // TODO: Integrate RenderGraph with viewport

        viewport_->end_render_pass(cmd);

        vkEndCommandBuffer(cmd);

        return cmd;
    }

    void Editor::end_frame(uint32_t image_index)
    {
        if (!initialized_)
        {
            return;
        }

        current_image_index_ = image_index;

        // Draw editor UI layout
        imgui_manager_->draw_editor_layout(viewport_.get());

        // End ImGui frame
        imgui_manager_->end_frame();
    }

    void Editor::render_to_command_buffer(VkCommandBuffer command_buffer)
    {
        if (!initialized_)
        {
            return;
        }

        imgui_manager_->render(command_buffer);
    }

    void Editor::update_stats(const ImGuiManager::StatsData& stats)
    {
        if (imgui_manager_)
        {
            imgui_manager_->update_stats(stats);
        }
    }

    bool Editor::is_viewport_focused() const
    {
        return imgui_manager_ ? imgui_manager_->is_viewport_focused() : false;
    }

    bool Editor::is_viewport_hovered() const
    {
        return imgui_manager_ ? imgui_manager_->is_viewport_hovered() : false;
    }

    VkRenderPass Editor::viewport_render_pass() const
    {
        return viewport_ ? viewport_->render_pass() : VK_NULL_HANDLE;
    }

    VkFramebuffer Editor::viewport_framebuffer() const
    {
        // SceneViewport doesn't expose framebuffer directly
        // It's managed internally by begin_render_pass/end_render_pass
        return VK_NULL_HANDLE;
    }

    void Editor::create_command_pool()
    {
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex        = device_->graphics_queue_family();

        VK_CHECK(vkCreateCommandPool(device_->device(), &pool_info, nullptr, &command_pool_));
    }

    void Editor::allocate_command_buffers(uint32_t count)
    {
        command_buffers_.resize(count);

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = command_pool_;
        alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount          = count;

        VK_CHECK(vkAllocateCommandBuffers(device_->device(), &alloc_info, command_buffers_.data()));
    }

    void Editor::recreate_render_pass(VkRenderPass render_pass, uint32_t image_count)
    {
        if (!initialized_ || !imgui_manager_)
            return;

        vkDeviceWaitIdle(device_->device());

        // Shutdown and reinitialize ImGui with new render pass
        imgui_manager_->shutdown();
        imgui_manager_->initialize(device_, window_, render_pass, image_count);

        logger::info("Editor render pass recreated");
    }
} // namespace vulkan_engine::editor
