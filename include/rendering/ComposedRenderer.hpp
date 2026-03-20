#pragma once

#include "rendering/SceneRenderer.hpp"
#include "rendering/UIRenderer.hpp"

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
     * @brief 组合渲染器 - 整合场景渲染和UI渲染
     *
     * 职责：
     * - 管理 SceneRenderer 和 UIRenderer 的生命周期
     * - 协调两个管线的渲染流程
     * - 提供统一的渲染接口给应用程序
     *
     * 渲染流程：
     * 1. begin_frame() - 获取 swap chain image，等待上一帧
     * 2. render_scene() - 渲染场景到 RenderTarget
     * 3. render_ui() - 渲染UI到 SwapChain（等待场景完成）
     * 4. end_frame() - 呈现到屏幕
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

            // 渲染回调
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

            // ========== 生命周期 ==========

            bool initialize(
                std::shared_ptr<platform::Window>      window,
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            void shutdown();

            // ========== 渲染循环 ==========

            /**
         * @brief 开始一帧渲染
         * @return 是否成功
         */
            bool begin_frame();

            /**
         * @brief 渲染场景
         * @param callback 场景渲染回调
         */
            void render_scene(SceneRenderCallback callback);

            /**
         * @brief 渲染UI
         * @param editor Editor 实例
         */
            void render_ui(editor::Editor& editor);

            /**
         * @brief 结束一帧并呈现
         */
            void end_frame();

            // ========== 场景控制 ==========

            /**
         * @brief 暂停场景渲染
         */
            void pause_scene() { scene_renderer_.pause(); }

            /**
         * @brief 恢复场景渲染
         */
            void resume_scene() { scene_renderer_.resume(); }

            /**
         * @brief 检查场景是否暂停
         */
            bool is_scene_paused() const { return scene_renderer_.is_paused(); }

            // ========== 渲染图管理 ==========

            RenderGraphBuilder& scene_render_graph_builder() { return scene_renderer_.render_graph_builder(); }
            RenderGraph&        scene_render_graph() { return scene_renderer_.render_graph(); }
            void                compile_scene_render_graph() { scene_renderer_.compile_render_graph(); }
            void                reset_scene_render_graph() { scene_renderer_.reset_render_graph(); }

            // ========== 访问子渲染器 ==========

            SceneRenderer& scene_renderer() { return scene_renderer_; }
            UIRenderer&    ui_renderer() { return ui_renderer_; }

            // ========== 渲染目标 ==========

            std::shared_ptr<RenderTarget> scene_render_target() const { return scene_renderer_.render_target(); }
            std::shared_ptr<Viewport>     scene_viewport() const { return scene_renderer_.viewport(); }

            // ========== 尺寸调整 ==========

            void resize(uint32_t width, uint32_t height);
            void resize_scene(uint32_t width, uint32_t height);
            void resize_ui(uint32_t width, uint32_t height);

            // ========== GPU 计时 ==========

            float get_scene_gpu_time_ms() const { return scene_renderer_.get_gpu_render_time_ms(); }

            // ========== 状态查询 ==========

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