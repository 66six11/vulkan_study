/**
 * @file IResourceBuilder.hpp
 * @brief 璧勬簮鏋勫缓鍣ㄦ帴鍙?
 */

#pragma once

#include "engine/rhi/vulkan/memory/IBuffer.hpp"
#include "engine/rhi/vulkan/memory/IImage.hpp"
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    // 鍓嶅悜澹版槑
    class IAllocator;

    /**
     * @brief Buffer 鏋勫缓鍣ㄦ帴鍙?
     * 
     * 浣跨敤 Builder 妯″紡鍒涘缓 Buffer
     */
    class IBufferBuilder
    {
        public:
            virtual ~IBufferBuilder() = default;

            // 鍩烘湰閰嶇疆
            virtual IBufferBuilder& size(uint64_t size) = 0;
            virtual IBufferBuilder& usage(uint32_t usage) = 0; // VkBufferUsageFlags

            // 鍐呭瓨灞炴€ч厤缃?
            virtual IBufferBuilder& hostVisible(bool persistentMap = false) = 0;
            virtual IBufferBuilder& hostCached() = 0;
            virtual IBufferBuilder& deviceLocal() = 0;

            // 鏋勫缓
            [[nodiscard]] virtual IBufferPtr build() = 0;

            // 棰勮閰嶇疆
            [[nodiscard]] virtual IBufferPtr createStagingBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createVertexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createIndexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createUniformBuffer(uint64_t size, bool persistentMap = true) = 0;
            [[nodiscard]] virtual IBufferPtr createStorageBuffer(uint64_t size, bool hostVisible = false) = 0;
    };

    using IBufferBuilderPtr = std::unique_ptr<IBufferBuilder>;

    /**
     * @brief Image 鏋勫缓鍣ㄦ帴鍙?
     */
    class IImageBuilder
    {
        public:
            virtual ~IImageBuilder() = default;

            // 灏哄閰嶇疆
            virtual IImageBuilder& width(uint32_t width) = 0;
            virtual IImageBuilder& height(uint32_t height) = 0;
            virtual IImageBuilder& depth(uint32_t depth) = 0;

            // 鏍煎紡鍜岀敤閫?
            virtual IImageBuilder& format(int format) = 0;        // VkFormat
            virtual IImageBuilder& usage(uint32_t usage) = 0;     // VkImageUsageFlags
            virtual IImageBuilder& samples(uint32_t samples) = 0; // VkSampleCountFlagBits

            // Mipmap 鍜屾暟缁?
            virtual IImageBuilder& mipLevels(uint32_t levels) = 0;
            virtual IImageBuilder& arrayLayers(uint32_t layers) = 0;

            // 鍐呭瓨灞炴€?
            virtual IImageBuilder& deviceLocal() = 0;
            virtual IImageBuilder& hostVisible() = 0;

            // 鏋勫缓
            [[nodiscard]] virtual IImagePtr build() = 0;

            // 棰勮閰嶇疆
            [[nodiscard]] virtual IImagePtr createColorAttachment(
                uint32_t width,
                uint32_t height,
                int      format,
                uint32_t mipLevels = 1,
                uint32_t samples   = 1
            ) = 0;

            [[nodiscard]] virtual IImagePtr createDepthAttachment(
                uint32_t width,
                uint32_t height,
                int      format,
                uint32_t samples = 1
            ) = 0;

            [[nodiscard]] virtual IImagePtr createTexture(
                uint32_t width,
                uint32_t height,
                int      format,
                uint32_t mipLevels   = 1,
                uint32_t arrayLayers = 1
            ) = 0;

            [[nodiscard]] virtual IImagePtr createCubemap(
                uint32_t size,
                int      format,
                uint32_t mipLevels = 1
            ) = 0;
    };

    using IImageBuilderPtr = std::unique_ptr<IImageBuilder>;

    /**
     * @brief 璧勬簮鏋勫缓鍣ㄥ伐鍘?
     */
    class IResourceBuilderFactory
    {
        public:
            virtual ~IResourceBuilderFactory() = default;

            [[nodiscard]] virtual IBufferBuilderPtr createBufferBuilder() = 0;
            [[nodiscard]] virtual IImageBuilderPtr  createImageBuilder() = 0;
    };
} // namespace vulkan_engine::vulkan::memory