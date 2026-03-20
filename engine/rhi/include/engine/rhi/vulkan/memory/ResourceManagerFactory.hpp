/**
 * @file ResourceManagerFactory.hpp
 * @brief 璧勬簮绠＄悊鍣ㄥ伐鍘?
 */

#pragma once

#include "engine/rhi/vulkan/memory/IAllocator.hpp"
#include <memory>

namespace vulkan_engine::vulkan
{
    class DeviceManager;
}

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief 璧勬簮绠＄悊鍣ㄧ被鍨?
     */
    enum class ResourceManagerType
    {
        VMA,    // Vulkan Memory Allocator
        Custom, // 鑷畾涔夊疄鐜?
        Mock    // 娴嬭瘯鐢?Mock
    };

    /**
     * @brief 璧勬簮绠＄悊鍣ㄥ垱寤洪厤缃?
     */
    struct ResourceManagerConfig
    {
        ResourceManagerType type = ResourceManagerType::VMA;

        // 閫氱敤閫夐」
        bool enableDefaultPools = true;
        bool enableBudget       = true;

        // VMA 鐗瑰畾閫夐」
        bool enableDefragmentation     = true;
        bool enableMemoryLeakDetection = false;

        // 璋冭瘯閫夐」
        bool recordAllocations = false;
        bool detailedLogging   = false;
    };

    /**
     * @brief 璧勬簮绠＄悊鍣ㄥ伐鍘?
     * 
     * 鐢ㄤ簬鍒涘缓涓嶅悓绫诲瀷鐨勮祫婧愮鐞嗗櫒瀹炰緥
     */
    class ResourceManagerFactory
    {
        public:
            /**
         * @brief 鍒涘缓璧勬簮绠＄悊鍣?
         */
            [[nodiscard]] static IResourceManagerPtr create(
                std::shared_ptr<DeviceManager> device,
                const ResourceManagerConfig&   config = {}
            );

            /**
         * @brief 鍒涘缓 VMA 璧勬簮绠＄悊鍣紙榛樿锛?
         */
            [[nodiscard]] static IResourceManagerPtr createVMA(
                std::shared_ptr<DeviceManager> device,
                bool                           enableDefaultPools = true
            );

            /**
         * @brief 鍒涘缓 Mock 璧勬簮绠＄悊鍣紙鐢ㄤ簬娴嬭瘯锛?
         */
            [[nodiscard]] static IResourceManagerPtr createMock();

            /**
         * @brief 鑾峰彇榛樿閰嶇疆
         */
            [[nodiscard]] static ResourceManagerConfig defaultConfig();

            /**
         * @brief 鑾峰彇璋冭瘯閰嶇疆
         */
            [[nodiscard]] static ResourceManagerConfig debugConfig();

            /**
         * @brief 鑾峰彇楂樻€ц兘閰嶇疆
         */
            [[nodiscard]] static ResourceManagerConfig performanceConfig();
    };
} // namespace vulkan_engine::vulkan::memory