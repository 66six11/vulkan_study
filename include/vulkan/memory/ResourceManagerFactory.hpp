/**
 * @file ResourceManagerFactory.hpp
 * @brief 资源管理器工厂
 */

#pragma once

#include "vulkan/memory/IAllocator.hpp"
#include <memory>

namespace vulkan_engine::vulkan
{
    class DeviceManager;
}

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief 资源管理器类型
     */
    enum class ResourceManagerType
    {
        VMA,    // Vulkan Memory Allocator
        Custom, // 自定义实现
        Mock    // 测试用 Mock
    };

    /**
     * @brief 资源管理器创建配置
     */
    struct ResourceManagerConfig
    {
        ResourceManagerType type = ResourceManagerType::VMA;

        // 通用选项
        bool enableDefaultPools = true;
        bool enableBudget       = true;

        // VMA 特定选项
        bool enableDefragmentation     = true;
        bool enableMemoryLeakDetection = false;

        // 调试选项
        bool recordAllocations = false;
        bool detailedLogging   = false;
    };

    /**
     * @brief 资源管理器工厂
     * 
     * 用于创建不同类型的资源管理器实例
     */
    class ResourceManagerFactory
    {
        public:
            /**
         * @brief 创建资源管理器
         */
            [[nodiscard]] static IResourceManagerPtr create(
                std::shared_ptr<DeviceManager> device,
                const ResourceManagerConfig&   config = {}
            );

            /**
         * @brief 创建 VMA 资源管理器（默认）
         */
            [[nodiscard]] static IResourceManagerPtr createVMA(
                std::shared_ptr<DeviceManager> device,
                bool                           enableDefaultPools = true
            );

            /**
         * @brief 创建 Mock 资源管理器（用于测试）
         */
            [[nodiscard]] static IResourceManagerPtr createMock();

            /**
         * @brief 获取默认配置
         */
            [[nodiscard]] static ResourceManagerConfig defaultConfig();

            /**
         * @brief 获取调试配置
         */
            [[nodiscard]] static ResourceManagerConfig debugConfig();

            /**
         * @brief 获取高性能配置
         */
            [[nodiscard]] static ResourceManagerConfig performanceConfig();
    };
} // namespace vulkan_engine::vulkan::memory