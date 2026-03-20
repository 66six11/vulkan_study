/**
 * @file IAllocator.hpp
 * @brief 内存分配器接口
 */

#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <cstdint>

namespace vulkan_engine::vulkan::memory
{
    // 前向声明
    struct AllocationInfo;
    struct AllocatorStats;
    struct Budget;

    /**
     * @brief 内存分配器接口
     * 
     * 提供抽象的内存分配功能，支持不同的实现（VMA、自定义分配器等）
     */
    class IAllocator
    {
        public:
            virtual ~IAllocator() = default;

            // 禁止拷贝
            IAllocator(const IAllocator&)            = delete;
            IAllocator& operator=(const IAllocator&) = delete;

            // 有效性检查
            [[nodiscard]] virtual bool isValid() const noexcept = 0;

            // 统计信息
            [[nodiscard]] virtual AllocatorStats getStats() const = 0;
            virtual void                         printStats() const = 0;

            // 预算查询
            [[nodiscard]] virtual std::vector<Budget> getHeapBudgets() const = 0;

            // 内存可用性检查
            [[nodiscard]] virtual bool isMemoryAvailable(uint64_t requiredBytes) const = 0;
    };

    using IAllocatorPtr = std::shared_ptr<IAllocator>;

    /**
     * @brief Buffer 分配器接口
     */
    class IBufferAllocator
    {
        public:
            virtual ~IBufferAllocator() = default;

            // 创建 Buffer
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createBuffer(
                uint64_t size,
                uint32_t usage,
                // VkBufferUsageFlags
                const void* allocInfo // 实现特定的分配信息
            ) = 0;

            // 销毁 Buffer（RAII 通常自动处理，但提供显式接口）
            virtual void destroyBuffer(std::shared_ptr<class IBuffer> buffer) = 0;

            // 创建特定类型的 Buffer
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createStagingBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createVertexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createIndexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createUniformBuffer(uint64_t size, bool persistentMap = true) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createStorageBuffer(uint64_t size, bool hostVisible = false) = 0;
    };

    /**
     * @brief Image 分配器接口
     */
    class IImageAllocator
    {
        public:
            virtual ~IImageAllocator() = default;

            // 创建 Image
            [[nodiscard]] virtual std::shared_ptr<class IImage> createImage(
                const void* imageInfo,
                // 实现特定的创建信息
                const void* allocInfo // 实现特定的分配信息
            ) = 0;

            // 销毁 Image
            virtual void destroyImage(std::shared_ptr<class IImage> image) = 0;

            // 创建特定类型的 Image
            [[nodiscard]] virtual std::shared_ptr<class IImage> createColorAttachment(
                uint32_t width,
                uint32_t height,
                int      format,
                // VkFormat
                uint32_t mipLevels = 1,
                uint32_t samples   = 1 // VkSampleCountFlagBits
            ) = 0;

            [[nodiscard]] virtual std::shared_ptr<class IImage> createDepthAttachment(
                uint32_t width,
                uint32_t height,
                int      format,
                // VkFormat
                uint32_t samples = 1
            ) = 0;

            [[nodiscard]] virtual std::shared_ptr<class IImage> createTexture(
                uint32_t width,
                uint32_t height,
                int      format,
                uint32_t mipLevels   = 1,
                uint32_t arrayLayers = 1
            ) = 0;

            [[nodiscard]] virtual std::shared_ptr<class IImage> createCubemap(
                uint32_t size,
                int      format,
                uint32_t mipLevels = 1
            ) = 0;
    };

    /**
     * @brief 通用资源管理器接口
     * 
     * 组合 Buffer 和 Image 分配功能
     */
    class IResourceManager : public IBufferAllocator, public IImageAllocator
    {
        public:
            ~IResourceManager() override = default;

            // 获取底层分配器
            [[nodiscard]] virtual IAllocatorPtr getAllocator() const = 0;

            // 统计信息
            [[nodiscard]] virtual AllocatorStats getStats() const = 0;

            // 预算查询
            [[nodiscard]] virtual std::vector<Budget> getHeapBudgets() const = 0;

            // 内存可用性检查
            [[nodiscard]] virtual bool isMemoryAvailable(uint64_t requiredBytes) const = 0;

            // 强制释放未使用的内存
            virtual void collectGarbage() = 0;

            // 碎片整理（如果支持）
            virtual void defragment() = 0;
    };

    using IResourceManagerPtr = std::shared_ptr<IResourceManager>;
} // namespace vulkan_engine::vulkan::memory