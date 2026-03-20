#include "engine/editor/Editor.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/rendering/resources/RenderTarget.hpp"
#include "engine/rendering/Viewport.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"

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

        initialized_ = false;
        logger::info("Editor shutdown");
    }

    void Editor::begin_frame()
    {
        if (!initialized_)
        {
            return;
        }

        // 搴旂敤 viewport 鐨勫欢杩?resize锛堝湪 ImGui 寮€濮嬪抚涔嬪墠锛?
        // 娉ㄦ剰锛氬鏋?deferred_resize_enabled_ 涓?true锛屽垯璺宠繃 resize 澶勭悊
        // 鐢辫皟鐢ㄨ€呭湪甯ц竟鐣岀粺涓€澶勭悊锛屼互閬垮厤璧勬簮绔炰簤
        if (viewport_ && !deferred_resize_enabled_)
        {
            // 妫€鏌ユ槸鍚︽湁寰呭鐞嗙殑 resize
            if (viewport_->is_resize_pending())
            {
                // 鑾峰彇鏂扮殑灏哄锛坮equest_resize涓缃殑pending灏哄锛?
                VkExtent2D new_extent = viewport_->pending_extent();

                // 1. 鍏堣 RenderTarget resize锛堣繖浼氶噸寤?Image/ImageView锛?
                viewport_->apply_pending_resize();

                // 2. 鐒跺悗鍒涘缓鏂扮殑 Framebuffer锛堜娇鐢ㄦ柊鐨?ImageView锛?
                if (viewport_resize_callback_)
                {
                    viewport_resize_callback_(new_extent.width, new_extent.height);
                }
            }
        }

        imgui_manager_->begin_frame();
    }

    void Editor::end_frame(uint32_t image_index)
    {
        (void)image_index; // Parameter kept for API compatibility

        if (!initialized_)
        {
            return;
        }

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