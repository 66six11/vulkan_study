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
    class SwapChain;
    class DepthBuffer;
    class RenderPassManager;
    class FramebufferPool;

    namespace memory
    {
        class VmaAllocator;
    }
}

namespace vulkan_engine::editor
{
    class Editor;
}

namespace vulkan_engine::rendering
{
    class Viewport;
    class RenderPassBase;

    /**
     * @brief 娓叉煋鍣ㄧ被 - Rendering Layer 鐨勬牳蹇冩帴鍙?
     * 
     * 鑱岃矗锛?
     * - 灏佽鎵€鏈?Vulkan 娓叉煋缁嗚妭锛屽悜 Application 灞傛彁渚涢珮灞傛帴鍙?
     * - 绠＄悊 RenderGraph銆佸悓姝ャ€佸懡浠ょ紦鍐层€佹覆鏌撶洰鏍?
     * - 澶勭悊绐楀彛澶у皬璋冩暣銆佽祫婧愰噸寤?
     * 
     * 璁捐鍘熷垯锛?
     * - Application 灞傚彧鑳介€氳繃姝ょ被杩涜娓叉煋锛屼笉鐩存帴鎺ヨЕ Vulkan API
     * - 鎵€鏈?Vulkan 瑁稿璞￠兘鍦ㄥ唴閮ㄧ鐞嗭紝涓嶆毚闇茬粰澶栭儴
     */
    class Renderer
    {
        public:
            struct Config
            {
                uint32_t width                = 1280;
                uint32_t height               = 720;
                bool     enable_gpu_timing    = true; // 鏄惁鍚敤 GPU 璁℃椂
                uint32_t max_frames_in_flight = 2;    // 鏈€澶у苟鍙戝抚鏁?
            };

            // 娓叉煋甯т笂涓嬫枃
            struct FrameContext
            {
                uint32_t frame_index;  // 褰撳墠甯х储寮?(0..max_frames_in_flight-1)
                uint32_t image_index;  // Swap chain image 绱㈠紩
                uint32_t width;        // 娓叉煋鐩爣瀹藉害
                uint32_t height;       // 娓叉煋鐩爣楂樺害
                float    delta_time;   // 甯ф椂闂?
                float    elapsed_time; // 绱鏃堕棿
            };

            // 娓叉煋闃舵鍥炶皟
            using SceneRenderCallback = std::function<void(vulkan::RenderCommandBuffer & cmd, const FrameContext & ctx)>;
            using UIRenderCallback    = std::function<void()>;

        public:
            Renderer();
            ~Renderer();

            // Non-copyable
            Renderer(const Renderer&)            = delete;
            Renderer& operator=(const Renderer&) = delete;

            // Movable
            Renderer(Renderer&& other) noexcept;
            Renderer& operator=(Renderer&& other) noexcept;

            // ========== 鐢熷懡鍛ㄦ湡 ==========

            /**
         * @brief 鍒濆鍖栨覆鏌撳櫒
         * @param device Vulkan 璁惧绠＄悊鍣?
         * @param swap_chain 浜ゆ崲閾?
         * @param config 娓叉煋鍣ㄩ厤缃?
         * @return 鏄惁鍒濆鍖栨垚鍔?
         */
            bool initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            /**
         * @brief 鍏抽棴娓叉煋鍣ㄥ苟娓呯悊璧勬簮
         */
            void shutdown();

            // ========== 娓叉煋寰幆 ==========

            /**
         * @brief 寮€濮嬩竴甯ф覆鏌?
         * @return 鏄惁鎴愬姛鑾峰彇 swap chain image
         */
            bool begin_frame();

            /**
         * @brief 娓叉煋鍦烘櫙鍒?RenderTarget
         * @param callback 鍦烘櫙娓叉煋鍥炶皟锛屽湪姝ゅ洖璋冧腑鎵ц RenderGraph
         */
            void render_scene(SceneRenderCallback callback);

            /**
         * @brief 娓叉煋 UI 鍒?SwapChain
         * @param editor Editor 瀹炰緥锛岀敤浜庢覆鏌?ImGui
         */
            void render_ui(editor::Editor& editor);

            /**
         * @brief 缁撴潫涓€甯ф覆鏌撳苟鎻愪氦鍒?GPU
         */
            void end_frame();

            // ========== 娓叉煋鍥剧鐞?==========

            /**
         * @brief 鑾峰彇 RenderGraph 鏋勫缓鍣?
         */
            RenderGraphBuilder& render_graph_builder() { return render_graph_.builder(); }

            /**
         * @brief 鑾峰彇 RenderGraph
         */
            RenderGraph& render_graph() { return render_graph_; }

            /**
         * @brief 缂栬瘧 RenderGraph锛堝湪娣诲姞鎵€鏈?Pass 鍚庤皟鐢級
         */
            void compile_render_graph();

            /**
         * @brief 閲嶇疆 RenderGraph
         */
            void reset_render_graph();

            // ========== 娓叉煋鐩爣 ==========

            /**
         * @brief 鑾峰彇鍦烘櫙娓叉煋鐩爣锛堢敤浜庣灞忔覆鏌擄級
         */
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            /**
         * @brief 鑾峰彇瑙嗗彛
         */
            std::shared_ptr<Viewport> viewport() const { return viewport_; }

            /**
         * @brief 鑾峰彇绂诲睆娓叉煋鐨?RenderPass锛堢敤浜庢潗璐ㄥ垵濮嬪寲锛?
         * @return VkRenderPass 绂诲睆娓叉煋鐨?RenderPass 鍙ユ焺
         */
            VkRenderPass get_offscreen_render_pass() const;

            /**
         * @brief 鑾峰彇浜ゆ崲閾炬覆鏌撶殑 RenderPass锛堝甫娣卞害锛?
         * @return VkRenderPass 浜ゆ崲閾炬覆鏌撶殑 RenderPass 鍙ユ焺
         */
            VkRenderPass get_present_render_pass() const;

            /**
         * @brief 璋冩暣娓叉煋鐩爣灏哄
         */
            void resize(uint32_t width, uint32_t height);

            /**
         * @brief 妫€鏌ユ槸鍚︽湁寰呭鐞嗙殑灏哄璋冩暣
         */
            bool is_resize_pending() const { return resize_pending_; }

            /**
         * @brief 搴旂敤寰呭鐞嗙殑灏哄璋冩暣锛堝簲鍦ㄥ抚杈圭晫璋冪敤锛?
         */
            void apply_pending_resize();

            // ========== GPU 璁℃椂 ==========

            /**
         * @brief 鑾峰彇涓婁竴甯х殑 GPU 娓叉煋鏃堕棿锛堟绉掞級
         * @return GPU 鏃堕棿锛屽鏋滄湭鍚敤鍒欒繑鍥?0
         */
            float get_gpu_render_time_ms() const { return gpu_render_time_ms_; }

            /**
         * @brief 鏄惁鍚敤 GPU 璁℃椂
         */
            bool is_gpu_timing_enabled() const { return !query_pools_.empty(); }

            // ========== 鐘舵€佹煡璇?==========

            /**
         * @brief 妫€鏌ユ槸鍚﹀凡鍒濆鍖?
         */
            bool is_initialized() const { return initialized_; }

            /**
         * @brief 鑾峰彇褰撳墠甯х储寮?
         */
            uint32_t current_frame() const { return current_frame_; }

            /**
         * @brief 鑾峰彇褰撳墠 swap chain image 绱㈠紩
         */
            uint32_t current_image() const { return current_image_; }

            /**
         * @brief 鑾峰彇閰嶇疆
         */
            const Config& config() const { return config_; }

            /**
         * @brief 鑾峰彇璁惧
         */
            std::shared_ptr<vulkan::DeviceManager> device() const { return device_; }

            /**
         * @brief 鑾峰彇浜ゆ崲閾?
         */
            std::shared_ptr<vulkan::SwapChain> swap_chain() const { return swap_chain_; }

        private:
            // 鍒濆鍖栧瓙绯荤粺
            bool initialize_vma_allocator();
            bool initialize_depth_buffer();
            bool initialize_render_pass_manager();
            bool initialize_frame_sync();
            bool initialize_command_pools();
            bool initialize_query_pools();
            bool initialize_framebuffer_pool();
            bool initialize_render_target();
            bool initialize_viewport();

            // 娓叉煋瀛愭楠?
            void record_scene_commands(SceneRenderCallback callback);
            void record_ui_commands_dynamic(editor::Editor& editor);
            void submit_scene_commands();
            void submit_ui_commands_dynamic();

            // GPU 璁℃椂
            void update_gpu_timing();
            void destroy_query_pools();

            // 閲嶅缓璧勬簮
            void recreate_swap_chain_resources();
            void recreate_render_target();

            // 娓呯悊
            void cleanup_resources();

        private:
            // 閰嶇疆
            Config config_;
            bool   initialized_ = false;

            // 璁惧 & 浜ゆ崲閾?
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<vulkan::SwapChain>     swap_chain_;

            // VMA 鍒嗛厤鍣?
            std::shared_ptr<vulkan::memory::VmaAllocator> vma_allocator_;

            // 娓叉煋鍥?
            RenderGraph render_graph_;

            // 娓叉煋鐩爣 & 瑙嗗彛
            std::shared_ptr<RenderTarget> render_target_;
            std::shared_ptr<Viewport>     viewport_;

            // 娣卞害缂撳啿
            std::unique_ptr<vulkan::DepthBuffer> depth_buffer_;

            // RenderPass 绠＄悊
            std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;

            // 鍚屾
            std::unique_ptr<vulkan::FrameSyncManager> frame_sync_;

            // 鍛戒护缂撳啿
            std::unique_ptr<vulkan::RenderCommandPool> scene_cmd_pool_;
            std::unique_ptr<vulkan::RenderCommandPool> ui_cmd_pool_;
            std::vector<vulkan::RenderCommandBuffer>   scene_cmd_buffers_;
            std::vector<vulkan::RenderCommandBuffer>   ui_cmd_buffers_;

            // Framebuffer 姹?
            std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_;

            // GPU 璁℃椂
            std::vector<VkQueryPool>  query_pools_;     // per-frame query pools
            std::vector<float>        gpu_frame_times_; // 鍘嗗彶璁板綍
            uint32_t                  gpu_time_write_index_ = 0;
            float                     gpu_render_time_ms_   = 0.0f;
            static constexpr uint32_t GPU_TIME_HISTORY_SIZE = 60;
            std::vector<bool>         query_pools_initialized_;

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