#pragma once

#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/render_graph/RenderGraphTypes.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include <memory>
#include <vector>

namespace vulkan_engine::rendering
{
    // 前向声明
    class RenderTarget;

    /**
     * @brief 视窗渲染通道
     * 管理渲染到纹理的完整流程，包括 RenderPass 生命周期
     */
    class ViewportRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string                   name = "ViewportRenderPass";
                std::shared_ptr<RenderTarget> render_target;                        // 渲染目标
                VkRenderPass                  render_pass         = VK_NULL_HANDLE; // 外部传入的 RenderPass (必须)
                bool                          clear_color         = true;
                bool                          clear_depth         = true;
                VkClearColorValue             clear_color_value   = {{0.1f, 0.1f, 0.1f, 1.0f}};
                float                         clear_depth_value   = 1.0f;
                uint32_t                      clear_stencil_value = 0;
            };

            explicit ViewportRenderPass(const Config& config);
            ~ViewportRenderPass() override;

            // 禁止拷贝
            ViewportRenderPass(const ViewportRenderPass&)            = delete;
            ViewportRenderPass& operator=(const ViewportRenderPass&) = delete;

            // 允许移动
            ViewportRenderPass(ViewportRenderPass&& other) noexcept            = default;
            ViewportRenderPass& operator=(ViewportRenderPass&& other) noexcept = default;

            // RenderGraphPass 接口
            void             setup(RenderGraphBuilder& builder) override;
            void             execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;
            std::string_view name() const override { return name_; }

            // 资源依赖
            std::vector<BufferHandle> get_buffer_inputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_inputs() const override { return {}; }
            std::vector<BufferHandle> get_buffer_outputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_outputs() const override;

            // 添加子渲染通道
            void add_sub_pass(std::unique_ptr<RenderPassBase> pass);

            // 获取渲染目标
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            // 设置外部 Framebuffer（由调用者管理生命周期）
            void          set_framebuffer(VkFramebuffer framebuffer) { framebuffer_ = framebuffer; }
            VkFramebuffer framebuffer() const { return framebuffer_; }

            // 重新创建 Framebuffer（尺寸变化时调用）
            void recreate_framebuffer();

        private:
            std::string                                  name_;
            Config                                       config_;
            std::shared_ptr<RenderTarget>                render_target_;
            std::vector<std::unique_ptr<RenderPassBase>> sub_passes_;

            // Vulkan 资源 (外部管理，ViewportRenderPass 不拥有所有权)
            VkRenderPass  render_pass_ = VK_NULL_HANDLE; // 由外部传入 (RenderPassManager)
            VkFramebuffer framebuffer_ = VK_NULL_HANDLE; // 由外部传入 (main.cpp)
    };
} // namespace vulkan_engine::rendering