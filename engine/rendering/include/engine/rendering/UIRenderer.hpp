#pragma once

#include "engine/rhi/vulkan/command/CommandBuffer.hpp"
#include "engine/rhi/vulkan/sync/Synchronization.hpp"

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
     * @brief UI娓叉煋鍣?- 鐙珛鐨勭紪杈戝櫒UI娓叉煋绠＄嚎
     *
     * 鑱岃矗锛?
     * - 绠＄悊缂栬緫鍣║I娓叉煋鍒?SwapChain
     * - 鎷ユ湁鐙珛鐨?CommandPool銆佸悓姝ュ璞?
     * - 鍙互浠庡満鏅覆鏌撶粨鏋滈噰鏍锋樉绀?
     * - 鐙珛浜庡満鏅覆鏌撹繍琛?
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

            // ========== 鐢熷懡鍛ㄦ湡 ==========

            bool initialize(
                std::shared_ptr<platform::Window>      window,
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            void shutdown();

            // ========== 娓叉煋寰幆 ==========

            /**
         * @brief 鑾峰彇涓嬩竴甯?swap chain image
         * @return 鏄惁鎴愬姛鑾峰彇
         */
            bool acquire_next_image();

            /**
         * @brief 娓叉煋UI鍒?SwapChain
         * @param editor Editor 瀹炰緥
         * @param scene_render_target 鍦烘櫙娓叉煋鐩爣锛堝彲閫夛紝鐢ㄤ簬鏄剧ず锛?
         * @param scene_finished_semaphore 鍦烘櫙娓叉煋瀹屾垚淇″彿閲忥紙鍙€夛紝鐢ㄤ簬绛夊緟鍦烘櫙锛?
         */
            void render(
                editor::Editor&               editor,
                std::shared_ptr<RenderTarget> scene_render_target      = nullptr,
                VkSemaphore                   scene_finished_semaphore = VK_NULL_HANDLE);

            /**
         * @brief 鍛堢幇鍒板睆骞?
         */
            void present();

            // ========== 灏哄璋冩暣 ==========

            void resize(uint32_t width, uint32_t height);
            bool is_resize_pending() const { return resize_pending_; }
            void apply_pending_resize();

            // ========== 鐘舵€佹煡璇?==========

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

            // UI涓撶敤鍚屾瀵硅薄
            // 浣跨敤 per-frame fence 鍜?acquire semaphore锛宲er-image render_finished semaphore
            struct FrameSync
            {
                std::unique_ptr<vulkan::Fence>     in_flight_fence;
                std::unique_ptr<vulkan::Semaphore> image_available_semaphore;
            };

            std::vector<FrameSync>                          frame_syncs_;
            std::vector<std::unique_ptr<vulkan::Semaphore>> render_finished_semaphores_; // per-image

            // 鍛戒护姹?
            std::unique_ptr<vulkan::RenderCommandPool> command_pool_;
            std::vector<vulkan::RenderCommandBuffer>   command_buffers_;

            // RenderPass 绠＄悊锛圛mGui 闇€瑕佷紶缁?RenderPass锛?
            std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;
            std::unique_ptr<vulkan::FramebufferPool>   framebuffer_pool_;
            VkRenderPass                               present_render_pass_ = VK_NULL_HANDLE;

            // 甯х姸鎬?
            uint32_t current_frame_ = 0;
            uint32_t current_image_ = 0;
            bool     frame_started_ = false;

            // 灏哄璋冩暣
            bool     resize_pending_ = false;
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
    };
} // namespace vulkan_engine::rendering