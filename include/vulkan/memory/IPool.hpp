/**
 * @file IPool.hpp
 * @brief 内存池接口
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief 内存池统计
     */
    struct PoolStats
    {
        uint64_t size            = 0; // 总大小
        uint64_t usedSize        = 0; // 已使用大小
        uint32_t allocationCount = 0; // 分配数量
        uint32_t blockCount      = 0; // 内存块数量
        uint32_t freeCount       = 0; // 空闲分配数量
    };

    /**
     * @brief 内存池接口
     * 
     * 提供预分配内存块的管理
     */
    class IPool
    {
        public:
            virtual ~IPool() = default;

            // 禁止拷贝
            IPool(const IPool&)            = delete;
            IPool& operator=(const IPool&) = delete;

            // 有效性检查
            [[nodiscard]] virtual bool isValid() const noexcept = 0;

            // 名称访问
            [[nodiscard]] virtual const std::string& name() const noexcept = 0;

            // 统计信息
            [[nodiscard]] virtual PoolStats getStats() const = 0;

            // 重置池（释放所有分配但保留内存块）
            virtual void reset() = 0;

            // 紧缩（释放未使用的内存块）
            virtual void trim() = 0;
    };

    using IPoolPtr = std::shared_ptr<IPool>;

    /**
     * @brief 内存池管理器接口
     */
    class IPoolManager
    {
        public:
            virtual ~IPoolManager() = default;

            // 预定义池类型
            enum class PoolType
            {
                Staging,      // 上传数据
                Vertex,       // 静态几何
                Index,        // 索引数据
                Uniform,      // Uniform 数据
                Texture,      // 纹理
                RenderTarget, // 渲染目标
                Dynamic,      // 动态更新
                Readback,     // GPU 回读
                Custom        // 自定义
            };

            // 获取池
            [[nodiscard]] virtual IPool*       getPool(PoolType type) = 0;
            [[nodiscard]] virtual const IPool* getPool(PoolType type) const = 0;

            // 创建自定义池
            [[nodiscard]] virtual IPoolPtr createPool(const void* createInfo) = 0; // 实现特定的创建信息

            // 销毁池
            virtual void destroyPool(IPoolPtr pool) = 0;

            // 获取所有池的统计
            virtual void printStats() const = 0;

            // 重置所有池
            virtual void resetAll() = 0;

            // 紧缩所有池
            virtual void trimAll() = 0;
    };

    using IPoolManagerPtr = std::shared_ptr<IPoolManager>;
} // namespace vulkan_engine::vulkan::memory