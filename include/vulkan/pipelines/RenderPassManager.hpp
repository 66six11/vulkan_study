#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vulkan_engine::vulkan
{
    /**
     * @brief RenderPass 配置键
     * 用于缓存和查找 RenderPass
     */
    struct RenderPassKey
    {
        VkFormat              color_format  = VK_FORMAT_UNDEFINED;
        VkFormat              depth_format  = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples       = VK_SAMPLE_COUNT_1_BIT;
        uint32_t              subpass_count = 1;
        bool                  offscreen     = false; // 最终布局不同

        bool operator==(const RenderPassKey& other) const
        {
            return color_format == other.color_format &&
                   depth_format == other.depth_format &&
                   samples == other.samples &&
                   subpass_count == other.subpass_count &&
                   offscreen == other.offscreen;
        }
    };

    struct RenderPassKeyHash
    {
        size_t operator()(const RenderPassKey& key) const
        {
            size_t hash = 0;
            hash_combine(hash, static_cast<uint32_t>(key.color_format));
            hash_combine(hash, static_cast<uint32_t>(key.depth_format));
            hash_combine(hash, static_cast<uint32_t>(key.samples));
            hash_combine(hash, key.subpass_count);
            hash_combine(hash, key.offscreen);
            return hash;
        }

        private:
            template <typename T> static void hash_combine(size_t& seed, const T& value)
            {
                seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
    };

    /**
     * @brief RenderPass 管理器
     * 集中创建和管理 RenderPass，提供缓存机制避免重复创建
     */
    class RenderPassManager
    {
        public:
            explicit RenderPassManager(std::shared_ptr<DeviceManager> device);
            ~RenderPassManager();

            // 禁止拷贝
            RenderPassManager(const RenderPassManager&)            = delete;
            RenderPassManager& operator=(const RenderPassManager&) = delete;

            // 允许移动
            RenderPassManager(RenderPassManager&& other) noexcept;
            RenderPassManager& operator=(RenderPassManager&& other) noexcept;

            /**
         * @brief 获取或创建 Present RenderPass（无深度）
         * @param color_format 颜色附件格式
         * @return RenderPass 句柄
         */
            VkRenderPass get_present_render_pass(VkFormat color_format);

            /**
         * @brief 获取或创建 Present RenderPass（带深度）
         * @param color_format 颜色附件格式
         * @param depth_format 深度附件格式
         * @return RenderPass 句柄
         */
            VkRenderPass get_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format);

            /**
         * @brief 获取或创建 Off-screen RenderPass
         * 最终布局为 SHADER_READ_ONLY_OPTIMAL，用于渲染到纹理
         * @param color_format 颜色附件格式
         * @param depth_format 深度附件格式
         * @return RenderPass 句柄
         */
            VkRenderPass get_offscreen_render_pass(VkFormat color_format, VkFormat depth_format);

            /**
         * @brief 获取或创建 Shadow Map RenderPass
         * @param depth_format 深度附件格式
         * @return RenderPass 句柄
         */
            VkRenderPass get_shadow_render_pass(VkFormat depth_format);

            /**
         * @brief 使用自定义配置获取或创建 RenderPass
         * @param key RenderPass 配置键
         * @return RenderPass 句柄
         */
            VkRenderPass get_or_create(const RenderPassKey& key);

            /**
         * @brief 清理未使用的 RenderPass
         * 应在适当时机调用以释放资源
         */
            void cleanup_unused();

            /**
         * @brief 清理所有 RenderPass
         */
            void clear();

        private:
            std::shared_ptr<DeviceManager>                                     device_;
            std::unordered_map<RenderPassKey, VkRenderPass, RenderPassKeyHash> cache_;

            // 创建具体的 RenderPass
            VkRenderPass create_present_render_pass(VkFormat color_format);
            VkRenderPass create_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format);
            VkRenderPass create_offscreen_render_pass(VkFormat color_format, VkFormat depth_format);
            VkRenderPass create_shadow_render_pass(VkFormat depth_format);

            // 通用的 RenderPass 创建辅助函数
            VkRenderPass create_render_pass(
                const std::vector<VkAttachmentDescription>& attachments,
                const std::vector<VkSubpassDescription>&    subpasses,
                const std::vector<VkSubpassDependency>&     dependencies);
    };
} // namespace vulkan_engine::vulkan