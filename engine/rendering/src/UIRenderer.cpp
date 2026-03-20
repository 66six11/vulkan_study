#include "engine/rendering/UIRenderer.hpp"
#include "engine/rendering/resources/RenderTarget.hpp"
#include "engine/editor/Editor.hpp"
#include "engine/rhi/vulkan/device/Device.hpp"
#include "engine/rhi/vulkan/device/SwapChain.hpp"
#include "engine/rhi/vulkan/pipelines/RenderPassManager.hpp"
#include "engine/rhi/vulkan/resources/Framebuffer.hpp"
#include "engine/platform/windowing/Window.hpp"
#include "engine/core/utils/Logger.hpp"

#include <imgui_impl_vulkan.h>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    UIRenderer::UIRenderer() = default;

    UIRenderer::~UIRenderer()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    UIRenderer::UIRenderer(UIRenderer&& other) noexcept
        : config_(std::move(other.config_))
        , initialized_(other.initialized_)
        , window_(std::move(other.window_))
        , device_(std::move(other.device_))
        , swap_chain_(std::move(other.swap_chain_))
        , frame_syncs_(std::move(other.frame_syncs_))
        , command_pool_(std::move(other.command_pool_))
        , command_buffers_(std::move(other.command_buffers_))
        , current_frame_(other.current_frame_)
        , current_image_(other.current_image_)
        , frame_started_(other.frame_started_)
        , resize_pending_(other.resize_pending_)
        , pending_width_(other.pending_width_)
        , pending_height_(other.pending_height_)
    {
        other.initialized_   = false;
        other.frame_started_ = false;
    }

    UIRenderer& UIRenderer::operator=(UIRenderer&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            config_          = std::move(other.config_);
            initialized_     = other.initialized_;
            window_          = std::move(other.window_);
            device_          = std::move(other.device_);
            swap_chain_      = std::move(other.swap_chain_);
            frame_syncs_     = std::move(other.frame_syncs_);
            command_pool_    = std::move(other.command_pool_);
            command_buffers_ = std::move(other.command_buffers_);
            current_frame_   = other.current_frame_;
            current_image_   = other.current_image_;
            frame_started_   = other.frame_started_;
            resize_pending_  = other.resize_pending_;
            pending_width_   = other.pending_width_;
            pending_height_  = other.pending_height_;

            other.initialized_   = false;
            other.frame_started_ = false;
        }
        return *this;
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool UIRenderer::initialize(
        std::shared_ptr<platform::Window>      window,
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<vulkan::SwapChain>     swap_chain,
        const Config&                          config)
    {
        if (initialized_)
        {
            logger::warn("UIRenderer already initialized");
            return true;
        }

        window_     = window;
        device_     = device;
        swap_chain_ = swap_chain;
        config_     = config;

        if (!window_ || !device_ || !swap_chain_)
        {
            logger::error("Window, device or swap chain is null");
            return false;
        }

        logger::info("Initializing UIRenderer...");

        if (!initialize_render_pass_manager()) return false;
        if (!initialize_framebuffer_pool()) return false;
        if (!initialize_command_pool()) return false;
        if (!initialize_frame_sync()) return false;

        initialized_ = true;
        logger::info("UIRenderer initialized successfully");
        return true;
    }

    bool UIRenderer::initialize_command_pool()
    {
        command_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device_,
                                                                    0,
                                                                    // graphics queue family
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        command_buffers_ = command_pool_->allocate(config_.max_frames_in_flight);

        logger::info("UI Command pool initialized");
        return true;
    }

    bool UIRenderer::initialize_render_pass_manager()
    {
        render_pass_manager_ = std::make_unique<vulkan::RenderPassManager>(device_);

        // 鍒涘缓鐢ㄤ簬鍛堢幇鐨?RenderPass锛堟棤娣卞害锛岀洿鎺ュ憟鐜板埌 SwapChain锛?
        present_render_pass_ = render_pass_manager_->get_present_render_pass(swap_chain_->format());

        logger::info("UI RenderPass initialized");
        return true;
    }

    bool UIRenderer::initialize_framebuffer_pool()
    {
        framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device_);

        // 涓?SwapChain images 鍒涘缓 Framebuffer
        std::vector<VkImageView> image_views;
        image_views.reserve(swap_chain_->image_count());
        for (uint32_t i = 0; i < swap_chain_->image_count(); ++i)
        {
            image_views.push_back(swap_chain_->get_image(i).view);
        }

        framebuffer_pool_->create_for_swap_chain(
                                                 present_render_pass_,
                                                 image_views,
                                                 swap_chain_->width(),
                                                 swap_chain_->height(),
                                                 VK_NULL_HANDLE); // No depth for UI

        logger::info("UI Framebuffer pool initialized with " + std::to_string(swap_chain_->image_count()) + " framebuffers");
        return true;
    }

    bool UIRenderer::initialize_frame_sync()
    {
        frame_syncs_.resize(config_.max_frames_in_flight);

        for (uint32_t i = 0; i < config_.max_frames_in_flight; ++i)
        {
            frame_syncs_[i].in_flight_fence           = std::make_unique<vulkan::Fence>(device_, true); // signaled
            frame_syncs_[i].image_available_semaphore = std::make_unique<vulkan::Semaphore>(device_);
        }

        // Create per-image render_finished semaphores
        uint32_t image_count = swap_chain_->image_count();
        render_finished_semaphores_.resize(image_count);
        for (uint32_t i = 0; i < image_count; ++i)
        {
            render_finished_semaphores_[i] = std::make_unique<vulkan::Semaphore>(device_);
        }

        logger::info("UI Frame sync objects created: " + std::to_string(config_.max_frames_in_flight) +
                     " frames, " + std::to_string(image_count) + " images");
        return true;
    }

    void UIRenderer::shutdown()
    {
        if (!initialized_ || !device_)
        {
            return;
        }

        logger::info("Shutting down UIRenderer...");

        // Wait for all GPU work to complete before cleanup
        vkDeviceWaitIdle(device_->device().handle());

        // Clear command buffers before pool
        command_buffers_.clear();

        // Reset all unique_ptr resources in reverse order of creation
        render_finished_semaphores_.clear();
        frame_syncs_.clear();

        framebuffer_pool_.reset();
        render_pass_manager_.reset();
        present_render_pass_ = VK_NULL_HANDLE;

        // Command pool must be destroyed after all command buffers are freed
        command_pool_.reset();

        swap_chain_.reset();
        device_.reset();
        window_.reset();

        initialized_ = false;
        logger::info("UIRenderer shutdown complete");
    }

    void UIRenderer::cleanup_resources()
    {
        // Clear command buffers before pool
        command_buffers_.clear();

        // Reset all unique_ptr resources in reverse order of creation
        render_finished_semaphores_.clear();
        frame_syncs_.clear();

        framebuffer_pool_.reset();
        render_pass_manager_.reset();
        present_render_pass_ = VK_NULL_HANDLE;

        // Command pool must be destroyed after all command buffers are freed
        command_pool_.reset();

        swap_chain_.reset();
        device_.reset();
        window_.reset();
    }

    // ============================================================================
    // Render Loop
    // ============================================================================

    bool UIRenderer::acquire_next_image()
    {
        if (!initialized_ || !swap_chain_)
        {
            return false;
        }

        // 鍦?fence wait 涔嬪墠搴旂敤 resize锛岀‘淇濅笂涓€甯у凡瀹屾垚
        if (resize_pending_)
        {
            apply_pending_resize();
            // After resize, skip this frame acquisition as swap chain was recreated
            // The next frame will acquire properly
        }

        auto& sync = frame_syncs_[current_frame_];

        // 绛夊緟涓婁竴甯у畬鎴?
        sync.in_flight_fence->wait();
        sync.in_flight_fence->reset();

        // Additional safety: ensure command buffer is not in use by resetting it
        // This is a no-op if the command buffer is already reset, but ensures clean state
        if (current_frame_ < command_buffers_.size())
        {
            vkResetCommandBuffer(command_buffers_[current_frame_].handle(), 0);
        }

        // 鑾峰彇涓嬩竴甯?image
        bool acquired = swap_chain_->acquire_next_image(
                                                        sync.image_available_semaphore->handle(),
                                                        VK_NULL_HANDLE,
                                                        current_image_);

        if (!acquired)
        {
            return false;
        }

        frame_started_ = true;
        return true;
    }

    void UIRenderer::render(editor::Editor& editor, std::shared_ptr<RenderTarget> scene_render_target, VkSemaphore scene_finished_semaphore)
    {
        (void)scene_render_target; // Unused for now, reserved for future use

        if (!frame_started_)
        {
            return;
        }

        record_commands(editor, scene_render_target);

        // 绛夊緟 image available 鍜屽満鏅畬鎴愶紙濡傛灉鏈夛級
        submit_commands(frame_syncs_[current_frame_].image_available_semaphore->handle(), scene_finished_semaphore);
    }

    void UIRenderer::record_commands(editor::Editor& editor, std::shared_ptr<RenderTarget> scene_render_target)
    {
        (void)scene_render_target; // Unused for now, reserved for future use

        auto&           cmd        = command_buffers_[current_frame_];
        VkCommandBuffer cmd_handle = cmd.handle();

        vkResetCommandBuffer(cmd_handle, 0);
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // 浣跨敤浼犵粺 RenderPass + Framebuffer锛圛mGui 闇€瑕侊級
        vulkan::Framebuffer* framebuffer = framebuffer_pool_->get_framebuffer(current_image_);
        if (!framebuffer)
        {
            logger::error("Failed to get framebuffer for image " + std::to_string(current_image_));
            return;
        }
        VkFramebuffer vk_framebuffer = framebuffer->handle();

        // 浣跨敤 framebuffer 鐨勫昂瀵革紙涓庡垱寤烘椂涓€鑷达級
        uint32_t fb_width  = framebuffer->width();
        uint32_t fb_height = framebuffer->height();

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass        = present_render_pass_;
        render_pass_info.framebuffer       = vk_framebuffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = {fb_width, fb_height};

        VkClearValue clear_value{};
        clear_value.color                = {0.2f, 0.2f, 0.2f, 1.0f};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues    = &clear_value;

        vkCmdBeginRenderPass(cmd_handle, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        // 娓叉煋 ImGui
        editor.render_to_command_buffer(cmd_handle);

        vkCmdEndRenderPass(cmd_handle);

        cmd.end();
    }

    void UIRenderer::submit_commands(VkSemaphore image_available_semaphore, VkSemaphore scene_finished_semaphore)
    {
        auto& cmd  = command_buffers_[current_frame_];
        auto& sync = frame_syncs_[current_frame_];

        // 鏋勫缓绛夊緟淇″彿閲忓垪琛?
        VkSemaphore          wait_semaphores[2];
        VkPipelineStageFlags wait_stages[2];
        uint32_t             wait_count = 0;

        // 1. Swap chain image 鍙敤
        wait_semaphores[wait_count] = image_available_semaphore;
        wait_stages[wait_count]     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        wait_count++;

        // 2. 鍦烘櫙娓叉煋瀹屾垚锛堝鏋滄湁锛?
        if (scene_finished_semaphore != VK_NULL_HANDLE)
        {
            wait_semaphores[wait_count] = scene_finished_semaphore;
            wait_stages[wait_count]     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            wait_count++;
        }

        // 浣跨敤 per-image render_finished semaphore
        VkSemaphore signal_semaphores[] = {render_finished_semaphores_[current_image_]->handle()};

        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount   = wait_count;
        submit_info.pWaitSemaphores      = wait_semaphores;
        submit_info.pWaitDstStageMask    = wait_stages;
        submit_info.commandBufferCount   = 1;
        VkCommandBuffer cmd_handle       = cmd.handle();
        submit_info.pCommandBuffers      = &cmd_handle;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        vkQueueSubmit(device_->graphics_queue().handle(), 1, &submit_info, sync.in_flight_fence->handle());
    }

    void UIRenderer::present()
    {
        if (!frame_started_ || !swap_chain_)
        {
            return;
        }

        // 浣跨敤 per-image render_finished semaphore
        VkSemaphore present_semaphore = render_finished_semaphores_[current_image_]->handle();
        swap_chain_->present(device_->graphics_queue().handle(), current_image_, present_semaphore);

        current_frame_ = (current_frame_ + 1) % config_.max_frames_in_flight;
        frame_started_ = false;
    }

    // ============================================================================
    // Resize
    // ============================================================================

    void UIRenderer::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        pending_width_  = width;
        pending_height_ = height;
        resize_pending_ = true;

        logger::info("UIRenderer resize marked: " + std::to_string(width) + "x" + std::to_string(height));
    }

    void UIRenderer::apply_pending_resize()
    {
        if (!swap_chain_)
        {
            return;
        }

        logger::info("Applying UIRenderer resize: " + std::to_string(pending_width_) + "x" + std::to_string(pending_height_));

        vkDeviceWaitIdle(device_->device().handle());

        // CRITICAL: Recreate swap chain first to get new-sized images
        if (!swap_chain_->recreate())
        {
            logger::error("Failed to recreate swap chain during resize");
            return;
        }

        logger::info("Swap chain recreated: " + std::to_string(swap_chain_->width()) + "x" + std::to_string(swap_chain_->height()));

        // Update render pass if format changed
        present_render_pass_ = render_pass_manager_->get_present_render_pass(swap_chain_->format());

        // Recreate framebuffer pool with swap chain dimensions (not pending dimensions)
        framebuffer_pool_->clear();
        std::vector<VkImageView> image_views;
        image_views.reserve(swap_chain_->image_count());
        for (uint32_t i = 0; i < swap_chain_->image_count(); ++i)
        {
            image_views.push_back(swap_chain_->get_image(i).view);
        }
        framebuffer_pool_->create_for_swap_chain(
                                                 present_render_pass_,
                                                 image_views,
                                                 swap_chain_->width(),
                                                 // Use actual swap chain dimensions
                                                 swap_chain_->height(),
                                                 VK_NULL_HANDLE);

        // Recreate swap chain resources (semaphores)
        recreate_swap_chain_resources();

        resize_pending_ = false;
    }

    void UIRenderer::recreate_swap_chain_resources()
    {
        // 閲嶆柊鍒涘缓 per-image render_finished semaphores
        uint32_t new_image_count = swap_chain_->image_count();
        render_finished_semaphores_.clear();
        render_finished_semaphores_.resize(new_image_count);
        for (uint32_t i = 0; i < new_image_count; ++i)
        {
            render_finished_semaphores_[i] = std::make_unique<vulkan::Semaphore>(device_);
        }

        logger::info("UIRenderer swap chain resources updated, " + std::to_string(new_image_count) + " images");
    }

    // ============================================================================
    // Getters
    // ============================================================================

    uint32_t UIRenderer::image_count() const
    {
        return swap_chain_ ? swap_chain_->image_count() : 0;
    }
} // namespace vulkan_engine::rendering
