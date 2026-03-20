#pragma once

#include "engine/rendering/render_graph/RenderGraphPass.hpp"
#include "engine/rendering/render_graph/RenderGraphTypes.hpp"
#include "engine/rhi/vulkan/command/CommandBuffer.hpp"
#include <memory>
#include <vector>

namespace vulkan_engine::rendering
{
    // 鍓嶅悜澹版槑
    class RenderTarget;

    /**
     * @brief 瑙嗙獥娓叉煋閫氶亾
     * 绠＄悊娓叉煋鍒扮汗鐞嗙殑瀹屾暣娴佺▼锛屽寘鎷?RenderPass 鐢熷懡鍛ㄦ湡
     */
    class ViewportRenderPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string                   name = "ViewportRenderPass";
                std::shared_ptr<RenderTarget> render_target;                        // 娓叉煋鐩爣
                VkRenderPass                  render_pass         = VK_NULL_HANDLE; // 澶栭儴浼犲叆鐨?RenderPass (蹇呴』)
                bool                          clear_color         = true;
                bool                          clear_depth         = true;
                VkClearColorValue             clear_color_value   = {{0.1f, 0.1f, 0.1f, 1.0f}};
                float                         clear_depth_value   = 1.0f;
                uint32_t                      clear_stencil_value = 0;
            };

            explicit ViewportRenderPass(const Config& config);
            ~ViewportRenderPass() override;

            // 绂佹鎷疯礉
            ViewportRenderPass(const ViewportRenderPass&)            = delete;
            ViewportRenderPass& operator=(const ViewportRenderPass&) = delete;

            // 鍏佽绉诲姩
            ViewportRenderPass(ViewportRenderPass&& other) noexcept            = default;
            ViewportRenderPass& operator=(ViewportRenderPass&& other) noexcept = default;

            // RenderGraphPass 鎺ュ彛
            void             setup(RenderGraphBuilder& builder) override;
            void             execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override;
            std::string_view name() const override { return name_; }

            // 璧勬簮渚濊禆
            std::vector<BufferHandle> get_buffer_inputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_inputs() const override { return {}; }
            std::vector<BufferHandle> get_buffer_outputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_outputs() const override;

            // 娣诲姞瀛愭覆鏌撻€氶亾
            void add_sub_pass(std::unique_ptr<RenderPassBase> pass);

            // 鑾峰彇娓叉煋鐩爣
            std::shared_ptr<RenderTarget> render_target() const { return render_target_; }

            // 璁剧疆澶栭儴 Framebuffer锛堢敱璋冪敤鑰呯鐞嗙敓鍛藉懆鏈燂級
            void          set_framebuffer(VkFramebuffer framebuffer) { framebuffer_ = framebuffer; }
            VkFramebuffer framebuffer() const { return framebuffer_; }

            // 閲嶆柊鍒涘缓 Framebuffer锛堝昂瀵稿彉鍖栨椂璋冪敤锛?
            void recreate_framebuffer();

        private:
            std::string                                  name_;
            Config                                       config_;
            std::shared_ptr<RenderTarget>                render_target_;
            std::vector<std::unique_ptr<RenderPassBase>> sub_passes_;

            // Vulkan 璧勬簮 (澶栭儴绠＄悊锛孷iewportRenderPass 涓嶆嫢鏈夋墍鏈夋潈)
            VkRenderPass  render_pass_ = VK_NULL_HANDLE; // 鐢卞閮ㄤ紶鍏?(RenderPassManager)
            VkFramebuffer framebuffer_ = VK_NULL_HANDLE; // 鐢卞閮ㄤ紶鍏?(main.cpp)
    };
} // namespace vulkan_engine::rendering