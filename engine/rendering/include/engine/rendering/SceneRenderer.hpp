#pragma once

#include "engine/rendering/render_graph/RenderGraph.hpp"
#include "engine/rendering/resources/RenderTarget.hpp"
#include "engine/rhi/vulkan/command/CommandBuffer.hpp"
#include "engine/rhi/vulkan/sync/Synchronization.hpp"

#include <memory>
#include <functional>

namespace vulkan_engine::vulkan
{
    class DeviceManager;
    class RenderPassManager;
    class FramebufferPool;

    namespace memory
    {
        class VmaAllocator;
    }
}

namespace vulkan_engine::rendering
{
    class Viewport;
    class RenderPassBase;

    /**
     * @brief 鍦烘櫙娓叉煋鍣?- 鐙珛鐨勬父鎴忎笘鐣屾覆鏌撶绾?
     *
     * 鑱岃矗锛?
     * - 绠＄悊鍦烘櫙娓叉煋鍒扮灞?RenderTarget
     * - 鎷ユ湁鐙珛鐨?RenderGraph銆丆ommandPool銆佸悓姝ュ璞?
     * - 鍙鐙珛鏆傚仠/鎭㈠
     * - 涓嶇洿鎺ヤ緷璧?UI 娓叉煋
     *
     * 璁捐鍘熷垯锛?
     * - 瀹屽叏鐙珛浜?UIRenderer
     * - 娓叉煋缁撴灉閫氳繃 RenderTarget 杈撳嚭
     */
    class SceneRenderer
    {
        public:
            struct Config
            {
                uint32_t width                = 1280;
                uint32_t height               = 720;
                bool     enable_gpu_timing    = true;
                uint32_t max_frames_in_flight = 2;
            };

            // 娓叉煋甯т笂涓嬫枃
            struct FrameContext
            {
                uint32_t frame_index;  // 褰撳墠甯х储寮?
                uint32_t width;        // 娓叉煋鐩爣瀹藉害
                uint32_t height;       // 娓叉煋鐩爣楂樺害
                float    delta_time;   // 甯ф椂闂?
                float    elapsed_time; // 绱鏃堕棿
            };

            // 娓叉煋鍥炶皟
            using SceneRenderCallback = std::function<void(vulkan::RenderCommandBuffer& cmd, const FrameContext& ctx)>;

        public:
            SceneRenderer();
            ~SceneRenderer();

            // Non-copyable
            SceneRenderer(const SceneRenderer&)            = delete;
            SceneRenderer& operator=(const SceneRenderer&) = delete;

            // Movable
            SceneRenderer(SceneRenderer&& other) noexcept;
            SceneRenderer& operator=(SceneRenderer&& other) noexcept;

            // ========== 鐢熷懡鍛ㄦ湡 ==========

            /**
             * @brief 鍒濆鍖栧満鏅覆鏌撳櫒
             * @param device Vulkan 璁惧绠＄悊鍣?
             * @param config 娓叉煋鍣ㄩ厤缃?
             * @return 鏄惁鍒濆鍖栨垚鍔?
             */
            bool initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                const Config&                          config = {});

            /**
             * @brief 鍏抽棴娓叉煋鍣ㄥ苟娓呯悊璧勬簮
             */
            void shutdown();

            // ========== 娓叉煋鎺у埗 ==========

            /**
             * @brief 寮€濮嬩竴甯у満鏅覆鏌?
             * @return 鏄惁鎴愬姛
             */
            bool begin_frame();

            /**
             * @brief 娓叉煋鍦烘櫙
             * @param callback 鍦烘櫙娓叉煋鍥炶皟
             */
            void render(SceneRenderCallback callback);

            /**
             * @brief 缁撴潫涓€甯у満鏅覆鏌?
             */
            void end_frame();

            /**
             * @brief 鏆傚仠鍦烘櫙娓叉煋
             */
            void pause() { paused_ = true; }

            /**
             * @brief 鎭㈠鍦烘櫙娓叉煋
             */
            void resume() { paused_ = false; }

            /**
             * @brief 妫€鏌ユ槸鍚︽殏鍋?
             */
            bool is_paused() const { return paused_; }

            // ========== RenderGraph 绠＄悊 ==========

            /**
             * @brief 鑾峰彇 RenderGraph 鏋勫缓鍣?
             */
            RenderGraphBuilder& render_graph_builder() { return render_graph_.builder(); }

            /**
             * @brief 鑾峰彇 RenderGraph
             */
            RenderGraph& render_graph() { return render_graph_; }

            /**
             * @brief 缂栬瘧 RenderGraph
             */
            void compile_render_graph();

            /**
             * @brief 閲嶇疆 RenderGraph
             */
            void reset_render_graph();

            // ========== 娓叉煋鐩爣 ==========

            /**
             * @brief 鑾峰彇鍦烘櫙娓叉煋鐩爣
             */
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            /**
             * @brief 鑾峰彇瑙嗗彛
             */
            std::shared_ptr<Viewport> viewport() const { return viewport_; }

            /**
             * @brief 鑾峰彇棰滆壊鍥惧儚瑙嗗浘锛堜緵UI閲囨牱锛?
             */
            VkImageView get_color_image_view() const;

            /**
             * @brief 鑾峰彇娣卞害鍥惧儚瑙嗗浘
             */
            VkImageView get_depth_image_view() const;

            /**
             * @brief 鑾峰彇鍦烘櫙瀹屾垚淇″彿閲忥紙渚沀I绛夊緟锛?
             */
            VkSemaphore get_scene_finished_semaphore() const;

            /**
             * @brief 璋冩暣娓叉煋鐩爣灏哄
             */
            void resize(uint32_t width, uint32_t height);

            /**
             * @brief 妫€鏌ユ槸鍚︽湁寰呭鐞嗙殑灏哄璋冩暣
             */
            bool is_resize_pending() const { return resize_pending_; }

            /**
             * @brief 搴旂敤寰呭鐞嗙殑灏哄璋冩暣
             */
            void apply_pending_resize();

            // ========== GPU 璁℃椂 ==========

            /**
             * @brief 鑾峰彇涓婁竴甯х殑 GPU 娓叉煋鏃堕棿锛堟绉掞級
             */
            float get_gpu_render_time_ms() const { return gpu_render_time_ms_; }

            /**
             * @brief 鏄惁鍚敤 GPU 璁℃椂
             */
            bool is_gpu_timing_enabled() const { return !query_pools_.empty(); }

            // ========== 鐘舵€佹煡璇?==========

            bool                                   is_initialized() const { return initialized_; }
            uint32_t                               current_frame() const { return current_frame_; }
            const Config&                          config() const { return config_; }
            std::shared_ptr<vulkan::DeviceManager> device() const { return device_; }

        private:
            bool initialize_vma_allocator();
            bool initialize_render_pass_manager();
            bool initialize_frame_sync();
            bool initialize_command_pool();
            bool initialize_query_pools();
            bool initialize_render_target();
            bool initialize_viewport();

            void record_commands(SceneRenderCallback callback);
            void submit_commands();

            void update_gpu_timing();
            void destroy_query_pools();
            void recreate_render_target();
            void cleanup_resources();

        private:
            Config config_;
            bool   initialized_ = false;
            bool   paused_      = false;

            std::shared_ptr<vulkan::DeviceManager>        device_;
            std::shared_ptr<vulkan::memory::VmaAllocator> vma_allocator_;

            RenderGraph                   render_graph_;
            std::shared_ptr<RenderTarget> render_target_;
            std::shared_ptr<Viewport>     viewport_;

            std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;

            // 鍦烘櫙涓撶敤鍚屾瀵硅薄
            struct FrameSync
            {
                std::unique_ptr<vulkan::Fence>     in_flight_fence;
                std::unique_ptr<vulkan::Semaphore> scene_finished_semaphore;
            };

            std::vector<FrameSync> frame_syncs_;

            // 鍛戒护姹?
            std::unique_ptr<vulkan::RenderCommandPool> command_pool_;
            std::vector<vulkan::RenderCommandBuffer>   command_buffers_;

            // GPU 璁℃椂
            std::vector<VkQueryPool>  query_pools_;
            std::vector<float>        gpu_frame_times_;
            uint32_t                  gpu_time_write_index_ = 0;
            float                     gpu_render_time_ms_   = 0.0f;
            static constexpr uint32_t GPU_TIME_HISTORY_SIZE = 60;
            std::vector<bool>         query_pools_initialized_;

            // 甯х姸鎬?
            uint32_t current_frame_ = 0;
            bool     frame_started_ = false;

            // 灏哄璋冩暣
            bool     resize_pending_ = false;
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
    };
} // namespace vulkan_engine::rendering
