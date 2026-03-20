#pragma once

#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // VMA 鍐呭瓨姹犵鐞嗗櫒
    class MemoryPool
    {
        public:
            struct CreateInfo
            {
                // 鍐呭瓨绫诲瀷锛堝鏋滆缃紝memoryTypeIndex 灏嗚鑷姩璁＄畻锛?
                std::optional<VkMemoryPropertyFlags> memoryProperties;

                // 鐩存帴鎸囧畾鍐呭瓨绫诲瀷绱㈠紩锛堜紭鍏堢骇楂樹簬 memoryProperties锛?
                uint32_t memoryTypeIndex = UINT32_MAX;

                // 鍧楀ぇ灏忥紙0 = 浣跨敤榛樿锛?
                VkDeviceSize blockSize = 0;

                // 鏈€灏?鏈€澶у潡鏁伴噺
                uint32_t minBlockCount = 0;
                uint32_t maxBlockCount = 0;

                // 瀵归綈瑕佹眰
                VkDeviceSize alignment = 0;

                // 鐢ㄩ€旀爣璇嗭紙鐢ㄤ簬缁熻锛?
                std::string name;

                // 鏄惁鍏佽鍒涘缓涓撶敤鍒嗛厤
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

            // 鑾峰彇鍘熺敓姹犲彞鏌?
            VmaPool handle() const noexcept { return pool_; }
            bool    isValid() const noexcept { return pool_ != VK_NULL_HANDLE; }

            // 鑾峰彇缁熻淇℃伅
            struct Stats
            {
                VkDeviceSize size;
                VkDeviceSize usedSize;
                uint32_t     allocationCount;
                uint32_t     blockCount;
            };

            Stats getStats() const;

            // 鍚嶇О
            const std::string& name() const { return name_; }

        private:
            std::shared_ptr<VmaAllocator> allocator_;
            VmaPool                       pool_ = VK_NULL_HANDLE;
            std::string                   name_;

            void cleanup() noexcept;
    };

    using MemoryPoolPtr = std::shared_ptr<MemoryPool>;

    // 棰勫畾涔夊唴瀛樻睜绫诲瀷
    enum class PoolType
    {
        Staging,      // Host-visible, 涓婁紶鏁版嵁
        Vertex,       // Device-local, 椤剁偣鏁版嵁
        Index,        // Device-local, 绱㈠紩鏁版嵁
        Uniform,      // Host-visible, uniform buffer
        Texture,      // Device-local, 绾圭悊
        RenderTarget, // Device-local, 娓叉煋鐩爣
        Dynamic,      // Host-visible, 鍔ㄦ€佹暟鎹?
        Readback      // Host-visible + cached, 鍥炶鏁版嵁
    };

    // 鍐呭瓨姹犵鐞嗗櫒 - 绠＄悊澶氫釜棰勫畾涔夋睜
    class MemoryPoolManager
    {
        public:
            explicit MemoryPoolManager(std::shared_ptr<VmaAllocator> allocator);
            ~MemoryPoolManager() = default;

            // 鍒涘缓棰勫畾涔夋睜
            void initializeDefaultPools();

            // 鑾峰彇姹?
            MemoryPool* getPool(PoolType type);
            VmaPool     getPoolHandle(PoolType type);

            // 鍒涘缓鑷畾涔夋睜
            MemoryPoolPtr createPool(const MemoryPool::CreateInfo& createInfo);

            // 鑾峰彇鎵€鏈夋睜鐨勭粺璁′俊鎭?
            void printStats() const;

            // 鑾峰彇姹犲悕绉?
            static const char* poolTypeToString(PoolType type);

        private:
            std::shared_ptr<VmaAllocator>               allocator_;
            std::unordered_map<PoolType, MemoryPoolPtr> pools_;
    };
} // namespace vulkan_engine::vulkan::memory