#include "editor/Editor.hpp"
#include "core/utils/Logger.hpp"
#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/resources/RenderTarget.hpp"
#include "rendering/Viewport.hpp"
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
        std::shared_ptr<platform::Window>        window,
        std::shared_ptr<vulkan::DeviceManager>   device,
        std::shared_ptr<vulkan::SwapChain>       swap_chain,
        std::shared_ptr<rendering::RenderTarget> render_target,
        std::shared_ptr<rendering::Viewport>     viewport)
    {
        window_        = window;
        device_        = device;
        swap_chain_    = swap_chain;
        render_target_ = render_target;
        viewport_      = viewport;

        // Get swap chain images for ImGui
        uint32_t image_count = swap_chain_->image_count();

        // Create ImGui manager
        imgui_manager_ = std::make_unique<ImGuiManager>();

        // Initialize ImGui with swap chain render pass
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
        // Note: render_target_ and viewport_ are managed externally, don't cleanup here

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

        // 应用 viewport 的延迟 resize（在 ImGui 开始帧之前）
        if (viewport_)
        {
            // 检查是否有待处理的 resize
            if (viewport_->is_resize_pending())
            {
                // 获取新的尺寸（request_resize中设置的pending尺寸）
                VkExtent2D new_extent = viewport_->pending_extent();

                // 1. 先让 RenderTarget resize（这会重建 Image/ImageView）
                viewport_->apply_pending_resize();

                // 2. 然后创建新的 Framebuffer（使用新的 ImageView）
                if (viewport_resize_callback_)
                {
                    viewport_resize_callback_(new_extent.width, new_extent.height);
                }
            }
        }

        imgui_manager_->begin_frame();
    }

    VkCommandBuffer Editor::render_scene(
        std::shared_ptr<rendering::RenderGraph> render_graph)
    {
        (void)render_graph;
        // Scene rendering is now handled in main.cpp using RenderTarget
        // This method is kept for backwards compatibility but returns nullptr
        return VK_NULL_HANDLE;
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

    bool Editor::is_viewport_content_hovered() const
    {
        return imgui_manager_ ? imgui_manager_->is_viewport_content_hovered() : false;
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
        if (!initialized_ || !imgui_manager_ || !device_)
            return;

        vkDeviceWaitIdle(device_->device());

        // According to ImGui Vulkan backend documentation:
        // When handling window resize, we need to update MinImageCount
        // and recreate the RenderPass if it has changed

        // Update MinImageCount for the new swap chain configuration
        ImGui_ImplVulkan_SetMinImageCount(image_count);

        // Note: If RenderPass has changed significantly, we may need to reinitialize
        // For now, we just update the min image count and let ImGui handle the rest
        (void)render_pass; // Render pass changes are handled by the main application

        logger::info("Editor render pass recreated with MinImageCount=" + std::to_string(image_count));
    }
} // namespace vulkan_engine::editor