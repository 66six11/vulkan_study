#pragma once

#include "engine/rendering/SceneRenderer.hpp"
#include "engine/rendering/UIRenderer.hpp"

#include <memory>

namespace vulkan_engine::vulkan
{
    class DeviceManager;
    class SwapChain;
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
    /**
     * @brief 缁勫悎娓叉煋鍣?- 鏁村悎鍦烘櫙娓叉煋鍜孶I娓叉煋
     *
     * 鑱岃矗锛?
     * - 绠＄悊 SceneRenderer 鍜?UIRenderer 鐨勭敓鍛藉懆鏈?
     * - 鍗忚皟涓や釜绠＄嚎鐨勬覆鏌撴祦绋?
     * - 鎻愪緵缁熶竴鐨勬覆鏌撴帴鍙ｇ粰搴旂敤绋嬪簭
     *
     * 娓叉煋娴佺▼锛?
     * 1. begin_frame() - 鑾峰彇 swap chain image锛岀瓑寰呬笂涓€甯?
     * 2. render_scene() - 娓叉煋鍦烘櫙鍒?RenderTarget
     * 3. render_ui() - 娓叉煋UI鍒?SwapChain锛堢瓑寰呭満鏅畬鎴愶級
     * 4. end_frame() - 鍛堢幇鍒板睆骞?
     */
    class ComposedRenderer
    {
        public:
            struct Config
            {
                uint32_t scene_width          = 1280;
                uint32_t scene_height         = 720;
                bool     enable_gpu_timing    = true;
                bool     enable_vsync         = true;
                uint32_t max_frames_in_flight = 2;
            };

            // 娓叉煋鍥炶皟
            using SceneRenderCallback = SceneRenderer::SceneRenderCallback;

        public:
            ComposedRenderer();
            ~ComposedRenderer();

            // Non-copyable
            ComposedRenderer(const ComposedRenderer&)            = delete;
            ComposedRenderer& operator=(const ComposedRenderer&) = delete;

            // Movable
            ComposedRenderer(ComposedRenderer&& other) noexcept;
            ComposedRenderer& operator=(ComposedRenderer&& other) noexcept;

            // ========== 鐢熷懡鍛ㄦ湡 ==========

            bool initialize(
                std::shared_ptr<platform::Window>      window,
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            void shutdown();

            // ========== 娓叉煋寰幆 ==========

            /**
         * @brief 寮€濮嬩竴甯ф覆鏌?
         * @return 鏄惁鎴愬姛
         */
            bool begin_frame();

            /**
         * @brief 娓叉煋鍦烘櫙
         * @param callback 鍦烘櫙娓叉煋鍥炶皟
         */
            void render_scene(SceneRenderCallback callback);

            /**
         * @brief 娓叉煋UI
         * @param editor Editor 瀹炰緥
         */
            void render_ui(editor::Editor& editor);

            /**
         * @brief 缁撴潫涓€甯у苟鍛堢幇
         */
            void end_frame();

            // ========== 鍦烘櫙鎺у埗 ==========

            /**
         * @brief 鏆傚仠鍦烘櫙娓叉煋
         */
            void pause_scene() { scene_renderer_.pause(); }

            /**
         * @brief 鎭㈠鍦烘櫙娓叉煋
         */
            void resume_scene() { scene_renderer_.resume(); }

            /**
         * @brief 妫€鏌ュ満鏅槸鍚︽殏鍋?
         */
            bool is_scene_paused() const { return scene_renderer_.is_paused(); }

            // ========== 娓叉煋鍥剧鐞?==========

            RenderGraphBuilder& scene_render_graph_builder() { return scene_renderer_.render_graph_builder(); }
            RenderGraph&        scene_render_graph() { return scene_renderer_.render_graph(); }
            void                compile_scene_render_graph() { scene_renderer_.compile_render_graph(); }
            void                reset_scene_render_graph() { scene_renderer_.reset_render_graph(); }

            // ========== 璁块棶瀛愭覆鏌撳櫒 ==========

            SceneRenderer& scene_renderer() { return scene_renderer_; }
            UIRenderer&    ui_renderer() { return ui_renderer_; }

            // ========== 娓叉煋鐩爣 ==========

            std::shared_ptr<RenderTarget> scene_render_target() const { return scene_renderer_.render_target(); }
            std::shared_ptr<Viewport>     scene_viewport() const { return scene_renderer_.viewport(); }

            // ========== 灏哄璋冩暣 ==========

            void resize(uint32_t width, uint32_t height);
            void resize_scene(uint32_t width, uint32_t height);
            void resize_ui(uint32_t width, uint32_t height);

            // ========== GPU 璁℃椂 ==========

            float get_scene_gpu_time_ms() const { return scene_renderer_.get_gpu_render_time_ms(); }

            // ========== 鐘舵€佹煡璇?==========

            bool     is_initialized() const { return initialized_; }
            uint32_t current_frame() const { return current_frame_; }
            uint32_t current_image() const { return ui_renderer_.current_image(); }

        private:
            bool initialized_ = false;

            SceneRenderer scene_renderer_;
            UIRenderer    ui_renderer_;

            uint32_t current_frame_ = 0;
    };
} // namespace vulkan_engine::rendering