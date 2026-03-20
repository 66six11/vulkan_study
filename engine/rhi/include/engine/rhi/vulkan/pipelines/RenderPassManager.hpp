#pragma once

#include "engine/rhi/vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vulkan_engine::vulkan
{
    /**
     * @brief RenderPass 閰嶇疆閿?
     * 鐢ㄤ簬缂撳瓨鍜屾煡鎵?RenderPass
     */
    struct RenderPassKey
    {
        VkFormat              color_format  = VK_FORMAT_UNDEFINED;
        VkFormat              depth_format  = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples       = VK_SAMPLE_COUNT_1_BIT;
        uint32_t              subpass_count = 1;
        bool                  offscreen     = false; // 鏈€缁堝竷灞€涓嶅悓

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
     * @brief RenderPass 绠＄悊鍣?
     * 闆嗕腑鍒涘缓鍜岀鐞?RenderPass锛屾彁渚涚紦瀛樻満鍒堕伩鍏嶉噸澶嶅垱寤?
     */
    class RenderPassManager
    {
        public:
            explicit RenderPassManager(std::shared_ptr<DeviceManager> device);
            ~RenderPassManager();

            // 绂佹鎷疯礉
            RenderPassManager(const RenderPassManager&)            = delete;
            RenderPassManager& operator=(const RenderPassManager&) = delete;

            // 鍏佽绉诲姩
            RenderPassManager(RenderPassManager&& other) noexcept;
            RenderPassManager& operator=(RenderPassManager&& other) noexcept;

            /**
         * @brief 鑾峰彇鎴栧垱寤?Present RenderPass锛堟棤娣卞害锛?
         * @param color_format 棰滆壊闄勪欢鏍煎紡
         * @return RenderPass 鍙ユ焺
         */
            VkRenderPass get_present_render_pass(VkFormat color_format);

            /**
         * @brief 鑾峰彇鎴栧垱寤?Present RenderPass锛堝甫娣卞害锛?
         * @param color_format 棰滆壊闄勪欢鏍煎紡
         * @param depth_format 娣卞害闄勪欢鏍煎紡
         * @return RenderPass 鍙ユ焺
         */
            VkRenderPass get_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format);

            /**
         * @brief 鑾峰彇鎴栧垱寤?Off-screen RenderPass
         * 鏈€缁堝竷灞€涓?SHADER_READ_ONLY_OPTIMAL锛岀敤浜庢覆鏌撳埌绾圭悊
         * @param color_format 棰滆壊闄勪欢鏍煎紡
         * @param depth_format 娣卞害闄勪欢鏍煎紡
         * @return RenderPass 鍙ユ焺
         */
            VkRenderPass get_offscreen_render_pass(VkFormat color_format, VkFormat depth_format);

            /**
         * @brief 鑾峰彇鎴栧垱寤?Shadow Map RenderPass
         * @param depth_format 娣卞害闄勪欢鏍煎紡
         * @return RenderPass 鍙ユ焺
         */
            VkRenderPass get_shadow_render_pass(VkFormat depth_format);

            /**
         * @brief 浣跨敤鑷畾涔夐厤缃幏鍙栨垨鍒涘缓 RenderPass
         * @param key RenderPass 閰嶇疆閿?
         * @return RenderPass 鍙ユ焺
         */
            VkRenderPass get_or_create(const RenderPassKey& key);

            /**
         * @brief 娓呯悊鏈娇鐢ㄧ殑 RenderPass
         * 搴斿湪閫傚綋鏃舵満璋冪敤浠ラ噴鏀捐祫婧?
         */
            void cleanup_unused();

            /**
         * @brief 娓呯悊鎵€鏈?RenderPass
         */
            void clear();

        private:
            std::shared_ptr<DeviceManager>                                     device_;
            std::unordered_map<RenderPassKey, VkRenderPass, RenderPassKeyHash> cache_;

            // 鍒涘缓鍏蜂綋鐨?RenderPass
            VkRenderPass create_present_render_pass(VkFormat color_format);
            VkRenderPass create_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format);
            VkRenderPass create_offscreen_render_pass(VkFormat color_format, VkFormat depth_format);
            VkRenderPass create_shadow_render_pass(VkFormat depth_format);

            // 閫氱敤鐨?RenderPass 鍒涘缓杈呭姪鍑芥暟
            VkRenderPass create_render_pass(
                const std::vector<VkAttachmentDescription>& attachments,
                const std::vector<VkSubpassDescription>&    subpasses,
                const std::vector<VkSubpassDependency>&     dependencies);
    };
} // namespace vulkan_engine::vulkan