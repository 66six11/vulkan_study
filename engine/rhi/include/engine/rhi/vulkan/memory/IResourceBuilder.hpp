/**
 * @file IResourceBuilder.hpp
 * @brief 资源构建器接口
 */

#pragma once

#include "vulkan/memory/IBuffer.hpp"
#include "vulkan/memory/IImage.hpp"
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    // 前向声明
    class IAllocator;

    /**
     * @brief Buffer 构建器接口
     * 
     * 使用 Builder 模式创建 Buffer
     */
    class IBufferBuilder
    {
        public:
            virtual ~IBufferBuilder() = default;

            // 基本配置
            virtual IBufferBuilder& size(uint64_t size) = 0;
            virtual IBufferBuilder& usage(uint32_t usage) = 0; // VkBufferUsageFlags

            // 内存属性配置
            virtual IBufferBuilder& hostVisible(bool persistentMap = false) = 0;
            virtual IBufferBuilder& hostCached() = 0;
            virtual IBufferBuilder& deviceLocal() = 0;

            // 构建
            [[nodiscard]] virtual IBufferPtr build() = 0;

            // 预设配置
            [[nodiscard]] virtual IBufferPtr createStagingBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createVertexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createIndexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual IBufferPtr createUniformBuffer(uint64_t size, bool persistentMap = true) = 0;
            [[nodiscard]] virtual IBufferPtr createStorageBuffer(uint64_t size, bool hostVisible = false) = 0;
    };

    using IBufferBuilderPtr = std::unique_ptr<IBufferBuilder>;

    /**
     * @brief Image 构建器接口
     */
    class IImageBuilder
    {
        public:
            virtual ~IImageBuilder() = default;

            // 尺寸配置
            virtual IImageBuilder& width(uint32_t width) = 0;
            virtual IImageBuilder& height(uint32_t height) = 0;
            virtual IImageBuilder& depth(uint32_t depth) = 0;

            // 格式和用途
            virtual IImageBuilder& format(int format) = 0;        // VkFormat
            virtual IImageBuilder& usage(uint32_t usage) = 0;     // VkImageUsageFlags
            virtual IImageBuilder& samples(uint32_t samples) = 0; // VkSampleCountFlagBits

            // Mipmap 和数组
            virtual IImageBuilder& mipLevels(uint32_t levels) = 0;
            virtual IImageBuilder& arrayLayers(uint32_t layers) = 0;

            // 内存属性
            virtual IImageBuilder& deviceLocal() = 0;
            virtual IImageBuilder& hostVisible() = 0;

            // 构建
            [[nodiscard]] virtual IImagePtr build() = 0;

            // 预设配置
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
     * @brief 资源构建器工厂
     */
    class IResourceBuilderFactory
    {
        public:
            virtual ~IResourceBuilderFactory() = default;

            [[nodiscard]] virtual IBufferBuilderPtr createBufferBuilder() = 0;
            [[nodiscard]] virtual IImageBuilderPtr  createImageBuilder() = 0;
    };
} // namespace vulkan_engine::vulkan::memory