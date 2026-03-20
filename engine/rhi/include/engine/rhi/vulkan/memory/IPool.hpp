/**
 * @file IPool.hpp
 * @brief 鍐呭瓨姹犳帴鍙?
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace vulkan_engine::vulkan::memory
{
    /**
     * @brief 鍐呭瓨姹犵粺璁?
     */
    struct PoolStats
    {
        uint64_t size            = 0; // 鎬诲ぇ灏?
        uint64_t usedSize        = 0; // 宸蹭娇鐢ㄥぇ灏?
        uint32_t allocationCount = 0; // 鍒嗛厤鏁伴噺
        uint32_t blockCount      = 0; // 鍐呭瓨鍧楁暟閲?
        uint32_t freeCount       = 0; // 绌洪棽鍒嗛厤鏁伴噺
    };

    /**
     * @brief 鍐呭瓨姹犳帴鍙?
     * 
     * 鎻愪緵棰勫垎閰嶅唴瀛樺潡鐨勭鐞?
     */
    class IPool
    {
        public:
            virtual ~IPool() = default;

            // 绂佹鎷疯礉
            IPool(const IPool&)            = delete;
            IPool& operator=(const IPool&) = delete;

            // 鏈夋晥鎬ф鏌?
            [[nodiscard]] virtual bool isValid() const noexcept = 0;

            // 鍚嶇О璁块棶
            [[nodiscard]] virtual const std::string& name() const noexcept = 0;

            // 缁熻淇℃伅
            [[nodiscard]] virtual PoolStats getStats() const = 0;

            // 閲嶇疆姹狅紙閲婃斁鎵€鏈夊垎閰嶄絾淇濈暀鍐呭瓨鍧楋級
            virtual void reset() = 0;

            // 绱х缉锛堥噴鏀炬湭浣跨敤鐨勫唴瀛樺潡锛?
            virtual void trim() = 0;
    };

    using IPoolPtr = std::shared_ptr<IPool>;

    /**
     * @brief 鍐呭瓨姹犵鐞嗗櫒鎺ュ彛
     */
    class IPoolManager
    {
        public:
            virtual ~IPoolManager() = default;

            // 棰勫畾涔夋睜绫诲瀷
            enum class PoolType
            {
                Staging,      // 涓婁紶鏁版嵁
                Vertex,       // 闈欐€佸嚑浣?
                Index,        // 绱㈠紩鏁版嵁
                Uniform,      // Uniform 鏁版嵁
                Texture,      // 绾圭悊
                RenderTarget, // 娓叉煋鐩爣
                Dynamic,      // 鍔ㄦ€佹洿鏂?
                Readback,     // GPU 鍥炶
                Custom        // 鑷畾涔?
            };

            // 鑾峰彇姹?
            [[nodiscard]] virtual IPool*       getPool(PoolType type) = 0;
            [[nodiscard]] virtual const IPool* getPool(PoolType type) const = 0;

            // 鍒涘缓鑷畾涔夋睜
            [[nodiscard]] virtual IPoolPtr createPool(const void* createInfo) = 0; // 瀹炵幇鐗瑰畾鐨勫垱寤轰俊鎭?

            // 閿€姣佹睜
            virtual void destroyPool(IPoolPtr pool) = 0;

            // 鑾峰彇鎵€鏈夋睜鐨勭粺璁?
            virtual void printStats() const = 0;

            // 閲嶇疆鎵€鏈夋睜
            virtual void resetAll() = 0;

            // 绱х缉鎵€鏈夋睜
            virtual void trimAll() = 0;
    };

    using IPoolManagerPtr = std::shared_ptr<IPoolManager>;
} // namespace vulkan_engine::vulkan::memory