#pragma once

#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/resources/RenderTarget.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

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
     * @brief 场景渲染器 - 独立的游戏世界渲染管线
     *
     * 职责：
     * - 管理场景渲染到离屏 RenderTarget
     * - 拥有独立的 RenderGraph、CommandPool、同步对象
     * - 可被独立暂停/恢复
     * - 不直接依赖 UI 渲染
     *
     * 设计原则：
     * - 完全独立于 UIRenderer
     * - 渲染结果通过 RenderTarget 输出
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

            // 渲染帧上下文
            struct FrameContext
            {
                uint32_t frame_index;  // 当前帧索引
                uint32_t width;        // 渲染目标宽度
                uint32_t height;       // 渲染目标高度
                float    delta_time;   // 帧时间
                float    elapsed_time; // 累计时间
            };

            // 渲染回调
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

            // ========== 生命周期 ==========

            /**
             * @brief 初始化场景渲染器
             * @param device Vulkan 设备管理器
             * @param config 渲染器配置
             * @return 是否初始化成功
             */
            bool initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                const Config&                          config = {});

            /**
             * @brief 关闭渲染器并清理资源
             */
            void shutdown();

            // ========== 渲染控制 ==========

            /**
             * @brief 开始一帧场景渲染
             * @return 是否成功
             */
            bool begin_frame();

            /**
             * @brief 渲染场景
             * @param callback 场景渲染回调
             */
            void render(SceneRenderCallback callback);

            /**
             * @brief 结束一帧场景渲染
             */
            void end_frame();

            /**
             * @brief 暂停场景渲染
             */
            void pause() { paused_ = true; }

            /**
             * @brief 恢复场景渲染
             */
            void resume() { paused_ = false; }

            /**
             * @brief 检查是否暂停
             */
            bool is_paused() const { return paused_; }

            // ========== RenderGraph 管理 ==========

            /**
             * @brief 获取 RenderGraph 构建器
             */
            RenderGraphBuilder& render_graph_builder() { return render_graph_.builder(); }

            /**
             * @brief 获取 RenderGraph
             */
            RenderGraph& render_graph() { return render_graph_; }

            /**
             * @brief 编译 RenderGraph
             */
            void compile_render_graph();

            /**
             * @brief 重置 RenderGraph
             */
            void reset_render_graph();

            // ========== 渲染目标 ==========

            /**
             * @brief 获取场景渲染目标
             */
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            /**
             * @brief 获取视口
             */
            std::shared_ptr<Viewport> viewport() const { return viewport_; }

            /**
             * @brief 获取颜色图像视图（供UI采样）
             */
            VkImageView get_color_image_view() const;

            /**
             * @brief 获取深度图像视图
             */
            VkImageView get_depth_image_view() const;

            /**
             * @brief 获取场景完成信号量（供UI等待）
             */
            VkSemaphore get_scene_finished_semaphore() const;

            /**
             * @brief 调整渲染目标尺寸
             */
            void resize(uint32_t width, uint32_t height);

            /**
             * @brief 检查是否有待处理的尺寸调整
             */
            bool is_resize_pending() const { return resize_pending_; }

            /**
             * @brief 应用待处理的尺寸调整
             */
            void apply_pending_resize();

            // ========== GPU 计时 ==========

            /**
             * @brief 获取上一帧的 GPU 渲染时间（毫秒）
             */
            float get_gpu_render_time_ms() const { return gpu_render_time_ms_; }

            /**
             * @brief 是否启用 GPU 计时
             */
            bool is_gpu_timing_enabled() const { return !query_pools_.empty(); }

            // ========== 状态查询 ==========

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

            // 场景专用同步对象
            struct FrameSync
            {
                std::unique_ptr<vulkan::Fence>     in_flight_fence;
                std::unique_ptr<vulkan::Semaphore> scene_finished_semaphore;
            };

            std::vector<FrameSync> frame_syncs_;

            // 命令池
            std::unique_ptr<vulkan::RenderCommandPool> command_pool_;
            std::vector<vulkan::RenderCommandBuffer>   command_buffers_;

            // GPU 计时
            std::vector<VkQueryPool>  query_pools_;
            std::vector<float>        gpu_frame_times_;
            uint32_t                  gpu_time_write_index_ = 0;
            float                     gpu_render_time_ms_   = 0.0f;
            static constexpr uint32_t GPU_TIME_HISTORY_SIZE = 60;
            std::vector<bool>         query_pools_initialized_;

            // 帧状态
            uint32_t current_frame_ = 0;
            bool     frame_started_ = false;

            // 尺寸调整
            bool     resize_pending_ = false;
            uint32_t pending_width_  = 0;
            uint32_t pending_height_ = 0;
    };
} // namespace vulkan_engine::rendering
