#pragma once

#include "vulkan/memory/VmaAllocator.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // VMA 内存池管理器
    class MemoryPool
    {
        public:
            struct CreateInfo
            {
                // 内存类型（如果设置，memoryTypeIndex 将被自动计算）
                std::optional<VkMemoryPropertyFlags> memoryProperties;

                // 直接指定内存类型索引（优先级高于 memoryProperties）
                uint32_t memoryTypeIndex = UINT32_MAX;

                // 块大小（0 = 使用默认）
                VkDeviceSize blockSize = 0;

                // 最小/最大块数量
                uint32_t minBlockCount = 0;
                uint32_t maxBlockCount = 0;

                // 对齐要求
                VkDeviceSize alignment = 0;

                // 用途标识（用于统计）
                std::string name;

                // 是否允许创建专用分配
                bool allowDedicatedAllocations = false;
            };

            MemoryPool(std::shared_ptr<VmaAllocator> allocator, const CreateInfo& createInfo);
            ~MemoryPool();

            // Non-copyable
            MemoryPool(const MemoryPool&)            = delete;
            MemoryPool& operator=(const MemoryPool&) = delete;

            // Movable
            MemoryPool(MemoryPool&& other) noexcept;
            MemoryPool& operator=(MemoryPool&& other) noexcept;

            // 获取原生池句柄
            VmaPool handle() const noexcept { return pool_; }
            bool    isValid() const noexcept { return pool_ != VK_NULL_HANDLE; }

            // 获取统计信息
            struct Stats
            {
                VkDeviceSize size;
                VkDeviceSize usedSize;
                uint32_t     allocationCount;
                uint32_t     blockCount;
            };

            Stats getStats() const;

            // 名称
            const std::string& name() const { return name_; }

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VmaPool                       pool_ = VK_NULL_HANDLE;
            std::string                   name_;

            void cleanup() noexcept;
    };

    using MemoryPoolPtr = std::shared_ptr<MemoryPool>;

    // 预定义内存池类型
    enum class PoolType
    {
        Staging,      // Host-visible, 上传数据
        Vertex,       // Device-local, 顶点数据
        Index,        // Device-local, 索引数据
        Uniform,      // Host-visible, uniform buffer
        Texture,      // Device-local, 纹理
        RenderTarget, // Device-local, 渲染目标
        Dynamic,      // Host-visible, 动态数据
        Readback      // Host-visible + cached, 回读数据
    };

    // 内存池管理器 - 管理多个预定义池
    class MemoryPoolManager
    {
        public:
            explicit MemoryPoolManager(std::shared_ptr<VmaAllocator> allocator);
            ~MemoryPoolManager() = default;

            // 创建预定义池
            void initializeDefaultPools();

            // 获取池
            MemoryPool* getPool(PoolType type);
            VmaPool     getPoolHandle(PoolType type);

            // 创建自定义池
            MemoryPoolPtr createPool(const MemoryPool::CreateInfo& createInfo);

            // 获取所有池的统计信息
            void printStats() const;

            // 获取池名称
            static const char* poolTypeToString(PoolType type);

        private:
            std::shared_ptr<VmaAllocator>               allocator_;
            std::unordered_map<PoolType, MemoryPoolPtr> pools_;
    };
} // namespace vulkan_engine::vulkan::memory