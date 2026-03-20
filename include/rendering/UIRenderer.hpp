#pragma once

#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

#include <memory>
#include <functional>

namespace vulkan_engine::vulkan
{
    class DeviceManager;
    class SwapChain;
    class RenderPassManager;
    class FramebufferPool;
}

namespace vulkan_engine::platform
{
    class Window;
}

namespace vulkan_engine::editor
{
    class Editor;
}

namespace vulkan_engine::rendering
{
    class RenderTarget;

    /**
     * @brief UI渲染器 - 独立的编辑器UI渲染管线
     *
     * 职责：
     * - 管理编辑器UI渲染到 SwapChain
     * - 拥有独立的 CommandPool、同步对象
     * - 可以从场景渲染结果采样显示
     * - 独立于场景渲染运行
     */
    class UIRenderer
    {
        public:
            struct Config
            {
                uint32_t max_frames_in_flight = 2;
                bool     enable_vsync         = true;
            };

        public:
            UIRenderer();
            ~UIRenderer();

            // Non-copyable
            UIRenderer(const UIRenderer&)            = delete;
            UIRenderer& operator=(const UIRenderer&) = delete;

            // Movable
            UIRenderer(UIRenderer&& other) noexcept;
            UIRenderer& operator=(UIRenderer&& other) noexcept;

            // ========== 生命周期 ==========

            bool initialize(
                std::shared_ptr<platform::Window>      window,
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            void shutdown();

            // ========== 渲染循环 ==========

            /**
         * @brief 获取下一帧 swap chain image
         * @return 是否成功获取
         */
            bool acquire_next_image();

            /**
         * @brief 渲染UI到 SwapChain
         * @param editor Editor 实例
         * @param scene_render_target 场景渲染目标（可选，用于显示）
         * @param scene_finished_semaphore 场景渲染完成信号量（可选，用于等待场景）
         */
            void render(
                editor::Editor&               editor,
                std::shared_ptr<RenderTarget> scene_render_target      = nullptr,
                VkSemaphore                   scene_finished_semaphore = VK_NULL_HANDLE);

            /**
         * @brief 呈现到屏幕
         */
            void present();

            // ========== 尺寸调整 ==========

            void resize(uint32_t width, uint32_t height);
            bool is_resize_pending() const { return resize_pending_; }
            void apply_pending_resize();

            // ========== 状态查询 ==========

            bool     is_initialized() const { return initialized_; }
            uint32_t current_image() const { return current_image_; }
            uint32_t image_count() const;

            std::shared_ptr<vulkan::DeviceManager> device() const { return device_; }
            std::shared_ptr<vulkan::SwapChain>     swap_chain() const { return swap_chain_; }

        private:
            bool initialize_command_pool();
            bool initialize_frame_sync();
            bool initialize_render_pass_manager();
            bool initialize_framebuffer_pool();

            void record_commands(editor::Editor& editor, std::shared_ptr<RenderTarget> scene_render_target);
            void submit_commands(VkSemaphore image_available_semaphore, VkSemaphore scene_finished_semaphore = VK_NULL_HANDLE);

            void recreate_swap_chain_resources();
            void cleanup_resources();

        private:
            Config config_;
            bool   initialized_ = false;

            std::shared_ptr<platform::Window>      window_;
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<vulkan::SwapChain>     swap_chain_;

            // UI专用同步对象
            // 使用 per-frame fence 和 acquire semaphore，per-image render_finished semaphore
            struct FrameSync
            {
                std::unique_ptr<vulkan::Fence>     in_flight_fence;
                std::unique_ptr<vulkan::Semaphore> image_available_semaphore;
            };

            std::vector<FrameSync>                          frame_syncs_;
            std::vector<std::unique_ptr<vulkan::Semaphore>> render_finished_semaphores_; // per-image

            // 命令池
            std::unique_ptr<vulkan::RenderCommandPool> command_pool_;
            std::vector<vulkan::RenderCommandBuffer>   command_buffers_;

            // RenderPass 管理（ImGui 需要传统 RenderPass）
            std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;
            std::unique_ptr<vulkan::FramebufferPool>   framebuffer_pool_;
            VkRenderPass                               present_render_pass_ = VK_NULL_HANDLE;

            // 帧状态
            uint32_t current_frame_ = 0;
            uint32_t current_image_ = 0;
            bool     frame_started_ = false;

            // 尺寸调整
            bool     resize_pending_ = false;
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
    };
} // namespace vulkan_engine::rendering