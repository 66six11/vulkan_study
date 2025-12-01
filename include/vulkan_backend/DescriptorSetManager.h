// include/vulkan_backend/DescriptorSetManager.h
#pragma once

#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>
#include "core/constants.h"
#include "vulkan_backend/VulkanDevice.h"

/**
 * @brief 描述符集管理类
 *
 * 统一管理 VkDescriptorPool 和 VkDescriptorSet 的分配与回收，
 * 简化 descriptor set 的获取与更新。
 */
class DescriptorSetManager
{
    public:
        /**
         * @brief 描述符池大小配置
         *
         * 使用“系数”来估算每个池中各类描述符的数量，
         * 实际数量 = maxSets * 对应系数。
         */
        struct PoolSizes
        {
            float sampler              = 0.5f;
            float combinedImageSampler = 4.0f;
            float sampledImage         = 4.0f;
            float storageImage         = 1.0f;
            float uniformBuffer        = 4.0f;
            float storageBuffer        = 1.0f;
            float uniformBufferDynamic = 1.0f;
            float storageBufferDynamic = 1.0f;
            float inputAttachment      = 0.5f;
        };

        explicit DescriptorSetManager(VulkanDevice& device);
        ~DescriptorSetManager();

        DescriptorSetManager(const DescriptorSetManager&)            = delete;
        DescriptorSetManager& operator=(const DescriptorSetManager&) = delete;

        /**
         * @brief 分配单个 descriptor set
         * @param layout 描述符集布局
         * @return 分配到的 VkDescriptorSet 句柄
         */
        VkDescriptorSet allocateSet(VkDescriptorSetLayout layout);

        /**
         * @brief 批量分配 descriptor set
         * @param layout 描述符集布局
         * @param count  需要的数量
         * @return 分配到的 VkDescriptorSet 数组
         */
        std::vector<VkDescriptorSet> allocateSets(VkDescriptorSetLayout layout, uint32_t count);

        /**
         * @brief 每帧重置（可选）
         *
         * 对所有池执行 vkResetDescriptorPool，
         * 以便重复使用描述符。
         * 注意：调用者必须确保 GPU 不再使用前一帧的 descriptor sets。
         */
        void resetFrame();

        /**
         * @brief 更新 descriptor set 内容
         *
         * 封装 vkUpdateDescriptorSets，方便调用端统一管理。
         * 该函数会自动将传入的 writes/copies 的 dstSet（以及必要时 srcSet）覆盖为目标 set，
         * 调用者无需自行设置 dstSet。
         *
         * @param set    目标 descriptor set
         * @param writes 写入操作数组（dstSet 会被本函数填充为 set）
         * @param copies 拷贝操作数组（srcSet/dstSet 中的 dstSet 会被覆盖为 set）
         */
        void updateDescriptorSet(
            VkDescriptorSet                 set,
            std::span<VkWriteDescriptorSet> writes,
            std::span<VkCopyDescriptorSet>  copies = {}) const;

        /**
         * @brief 设置默认池大小配置
         * @param sizes 新的默认配置
         */
        void setDefaultPoolSizes(const PoolSizes& sizes) { defaultPoolSizes_ = sizes; }

    private:
        struct Pool
        {
            VkDescriptorPool pool{VK_NULL_HANDLE};
            uint32_t         maxSets{0};
            uint32_t         usedSets{0};
        };

        struct LayoutPools
        {
            std::vector<Pool> pools;
        };

        // VkDescriptorSetLayout 的哈希，用于 unordered_map
        struct LayoutHash
        {
            size_t operator()(VkDescriptorSetLayout layout) const noexcept
            {
                return std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(layout));
            }
        };

        VulkanDevice& device_;
        PoolSizes     defaultPoolSizes_{};

        // 按 VkDescriptorSetLayout 分组管理池
        std::unordered_map<VkDescriptorSetLayout, LayoutPools, LayoutHash> layoutPools_;
        std::mutex                                                         mutex_;

    private:
        /**
         * @brief 获取或创建适合指定 layout 的池
         *
         * 如果现有池都满了，则创建一个新的池并加入列表。
         */
        Pool& getOrCreatePool(VkDescriptorSetLayout layout);

        /**
         * @brief 创建一个新的 VkDescriptorPool
         * @param sizes   池大小配置
         * @param maxSets 池中最大可分配的 set 数量
         * @return 创建的 VkDescriptorPool
         */
        VkDescriptorPool createPool(const PoolSizes& sizes, uint32_t maxSets) const;
};
