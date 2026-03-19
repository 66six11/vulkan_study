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
     * @brief 渲染器类 - Rendering Layer 的核心接口
     * 
     * 职责：
     * - 封装所有 Vulkan 渲染细节，向 Application 层提供高层接口
     * - 管理 RenderGraph、同步、命令缓冲、渲染目标
     * - 处理窗口大小调整、资源重建
     * 
     * 设计原则：
     * - Application 层只能通过此类进行渲染，不直接接触 Vulkan API
     * - 所有 Vulkan 裸对象都在内部管理，不暴露给外部
     */
    class Renderer
    {
        public:
            struct Config
            {
                uint32_t width                = 1280;
                uint32_t height               = 720;
                bool     enable_gpu_timing    = true; // 是否启用 GPU 计时
                uint32_t max_frames_in_flight = 2;    // 最大并发帧数
            };

            // 渲染帧上下文
            struct FrameContext
            {
                uint32_t frame_index;  // 当前帧索引 (0..max_frames_in_flight-1)
                uint32_t image_index;  // Swap chain image 索引
                uint32_t width;        // 渲染目标宽度
                uint32_t height;       // 渲染目标高度
                float    delta_time;   // 帧时间
                float    elapsed_time; // 累计时间
            };

            // 渲染阶段回调
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

            // ========== 生命周期 ==========

            /**
         * @brief 初始化渲染器
         * @param device Vulkan 设备管理器
         * @param swap_chain 交换链
         * @param config 渲染器配置
         * @return 是否初始化成功
         */
            bool initialize(
                std::shared_ptr<vulkan::DeviceManager> device,
                std::shared_ptr<vulkan::SwapChain>     swap_chain,
                const Config&                          config = {});

            /**
         * @brief 关闭渲染器并清理资源
         */
            void shutdown();

            // ========== 渲染循环 ==========

            /**
         * @brief 开始一帧渲染
         * @return 是否成功获取 swap chain image
         */
            bool begin_frame();

            /**
         * @brief 渲染场景到 RenderTarget
         * @param callback 场景渲染回调，在此回调中执行 RenderGraph
         */
            void render_scene(SceneRenderCallback callback);

            /**
         * @brief 渲染 UI 到 SwapChain
         * @param editor Editor 实例，用于渲染 ImGui
         */
            void render_ui(editor::Editor& editor);

            /**
         * @brief 结束一帧渲染并提交到 GPU
         */
            void end_frame();

            // ========== 渲染图管理 ==========

            /**
         * @brief 获取 RenderGraph 构建器
         */
            RenderGraphBuilder& render_graph_builder() { return render_graph_.builder(); }

            /**
         * @brief 获取 RenderGraph
         */
            RenderGraph& render_graph() { return render_graph_; }

            /**
         * @brief 编译 RenderGraph（在添加所有 Pass 后调用）
         */
            void compile_render_graph();

            /**
         * @brief 重置 RenderGraph
         */
            void reset_render_graph();

            // ========== 渲染目标 ==========

            /**
         * @brief 获取场景渲染目标（用于离屏渲染）
         */
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            /**
         * @brief 获取视口
         */
            std::shared_ptr<Viewport> viewport() const { return viewport_; }

            /**
         * @brief 获取离屏渲染的 RenderPass（用于材质初始化）
         * @return VkRenderPass 离屏渲染的 RenderPass 句柄
         */
            VkRenderPass get_offscreen_render_pass() const;

            /**
         * @brief 获取交换链渲染的 RenderPass（带深度）
         * @return VkRenderPass 交换链渲染的 RenderPass 句柄
         */
            VkRenderPass get_present_render_pass() const;

            /**
         * @brief 调整渲染目标尺寸
         */
            void resize(uint32_t width, uint32_t height);

            /**
         * @brief 检查是否有待处理的尺寸调整
         */
            bool is_resize_pending() const { return resize_pending_; }

            /**
         * @brief 应用待处理的尺寸调整（应在帧边界调用）
         */
            void apply_pending_resize();

            // ========== GPU 计时 ==========

            /**
         * @brief 获取上一帧的 GPU 渲染时间（毫秒）
         * @return GPU 时间，如果未启用则返回 0
         */
            float get_gpu_render_time_ms() const { return gpu_render_time_ms_; }

            /**
         * @brief 是否启用 GPU 计时
         */
            bool is_gpu_timing_enabled() const { return !query_pools_.empty(); }

            // ========== 状态查询 ==========

            /**
         * @brief 检查是否已初始化
         */
            bool is_initialized() const { return initialized_; }

            /**
         * @brief 获取当前帧索引
         */
            uint32_t current_frame() const { return current_frame_; }

            /**
         * @brief 获取当前 swap chain image 索引
         */
            uint32_t current_image() const { return current_image_; }

            /**
         * @brief 获取配置
         */
            const Config& config() const { return config_; }

            /**
         * @brief 获取设备
         */
            std::shared_ptr<vulkan::DeviceManager> device() const { return device_; }

            /**
         * @brief 获取交换链
         */
            std::shared_ptr<vulkan::SwapChain> swap_chain() const { return swap_chain_; }

        private:
            // 初始化子系统
            bool initialize_vma_allocator();
            bool initialize_depth_buffer();
            bool initialize_render_pass_manager();
            bool initialize_frame_sync();
            bool initialize_command_pools();
            bool initialize_query_pools();
            bool initialize_framebuffer_pool();
            bool initialize_render_target();
            bool initialize_viewport();

            // 渲染子步骤
            void record_scene_commands(SceneRenderCallback callback);
            void record_ui_commands_dynamic(editor::Editor& editor);
            void submit_scene_commands();
            void submit_ui_commands_dynamic();

            // GPU 计时
            void update_gpu_timing();
            void destroy_query_pools();

            // 重建资源
            void recreate_swap_chain_resources();
            void recreate_render_target();

            // 清理
            void cleanup_resources();

        private:
            // 配置
            Config config_;
            bool   initialized_ = false;

            // 设备 & 交换链
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::shared_ptr<vulkan::SwapChain>     swap_chain_;

            // VMA 分配器
            std::shared_ptr<vulkan::memory::VmaAllocator> vma_allocator_;

            // 渲染图
            RenderGraph render_graph_;

            // 渲染目标 & 视口
            std::shared_ptr<RenderTarget> render_target_;
            std::shared_ptr<Viewport>     viewport_;

            // 深度缓冲
            std::unique_ptr<vulkan::DepthBuffer> depth_buffer_;

            // RenderPass 管理
            std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;

            // 同步
            std::unique_ptr<vulkan::FrameSyncManager> frame_sync_;

            // 命令缓冲
            std::unique_ptr<vulkan::RenderCommandPool> scene_cmd_pool_;
            std::unique_ptr<vulkan::RenderCommandPool> ui_cmd_pool_;
            std::vector<vulkan::RenderCommandBuffer>   scene_cmd_buffers_;
            std::vector<vulkan::RenderCommandBuffer>   ui_cmd_buffers_;

            // Framebuffer 池
            std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_;

            // GPU 计时
            std::vector<VkQueryPool>  query_pools_;     // per-frame query pools
            std::vector<float>        gpu_frame_times_; // 历史记录
            uint32_t                  gpu_time_write_index_ = 0;
            float                     gpu_render_time_ms_   = 0.0f;
            static constexpr uint32_t GPU_TIME_HISTORY_SIZE = 60;
            std::vector<bool>         query_pools_initialized_;

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