/**
 * @file IAllocator.hpp
 * @brief 鍐呭瓨鍒嗛厤鍣ㄦ帴鍙?
 */

#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <cstdint>

namespace vulkan_engine::vulkan::memory
{
    // 鍓嶅悜澹版槑
    struct AllocationInfo;
    struct AllocatorStats;
    struct Budget;

    /**
     * @brief 鍐呭瓨鍒嗛厤鍣ㄦ帴鍙?
     * 
     * 鎻愪緵鎶借薄鐨勫唴瀛樺垎閰嶅姛鑳斤紝鏀寔涓嶅悓鐨勫疄鐜帮紙VMA銆佽嚜瀹氫箟鍒嗛厤鍣ㄧ瓑锛?
     */
    class IAllocator
    {
        public:
            virtual ~IAllocator() = default;

            // 绂佹鎷疯礉
            IAllocator(const IAllocator&)            = delete;
            IAllocator& operator=(const IAllocator&) = delete;

            // 鏈夋晥鎬ф鏌?
            [[nodiscard]] virtual bool isValid() const noexcept = 0;

            // 缁熻淇℃伅
            [[nodiscard]] virtual AllocatorStats getStats() const = 0;
            virtual void                         printStats() const = 0;

            // 棰勭畻鏌ヨ
            [[nodiscard]] virtual std::vector<Budget> getHeapBudgets() const = 0;

            // 鍐呭瓨鍙敤鎬ф鏌?
            [[nodiscard]] virtual bool isMemoryAvailable(uint64_t requiredBytes) const = 0;
    };

    using IAllocatorPtr = std::shared_ptr<IAllocator>;

    /**
     * @brief Buffer 鍒嗛厤鍣ㄦ帴鍙?
     */
    class IBufferAllocator
    {
        public:
            virtual ~IBufferAllocator() = default;

            // 鍒涘缓 Buffer
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createBuffer(
                uint64_t size,
                uint32_t usage,
                // VkBufferUsageFlags
                const void* allocInfo // 瀹炵幇鐗瑰畾鐨勫垎閰嶄俊鎭?
            ) = 0;

            // 閿€姣?Buffer锛圧AII 閫氬父鑷姩澶勭悊锛屼絾鎻愪緵鏄惧紡鎺ュ彛锛?
            virtual void destroyBuffer(std::shared_ptr<class IBuffer> buffer) = 0;

            // 鍒涘缓鐗瑰畾绫诲瀷鐨?Buffer
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createStagingBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createVertexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createIndexBuffer(uint64_t size) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createUniformBuffer(uint64_t size, bool persistentMap = true) = 0;
            [[nodiscard]] virtual std::shared_ptr<class IBuffer> createStorageBuffer(uint64_t size, bool hostVisible = false) = 0;
    };

    /**
     * @brief Image 鍒嗛厤鍣ㄦ帴鍙?
     */
    class IImageAllocator
    {
        public:
            virtual ~IImageAllocator() = default;

            // 鍒涘缓 Image
            [[nodiscard]] virtual std::shared_ptr<class IImage> createImage(
                const void* imageInfo,
                // 瀹炵幇鐗瑰畾鐨勫垱寤轰俊鎭?
                const void* allocInfo // 瀹炵幇鐗瑰畾鐨勫垎閰嶄俊鎭?
            ) = 0;

            // 閿€姣?Image
            virtual void destroyImage(std::shared_ptr<class IImage> image) = 0;

            // 鍒涘缓鐗瑰畾绫诲瀷鐨?Image
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
     * @brief 閫氱敤璧勬簮绠＄悊鍣ㄦ帴鍙?
     * 
     * 缁勫悎 Buffer 鍜?Image 鍒嗛厤鍔熻兘
     */
    class IResourceManager : public IBufferAllocator, public IImageAllocator
    {
        public:
            ~IResourceManager() override = default;

            // 鑾峰彇搴曞眰鍒嗛厤鍣?
            [[nodiscard]] virtual IAllocatorPtr getAllocator() const = 0;

            // 缁熻淇℃伅
            [[nodiscard]] virtual AllocatorStats getStats() const = 0;

            // 棰勭畻鏌ヨ
            [[nodiscard]] virtual std::vector<Budget> getHeapBudgets() const = 0;

            // 鍐呭瓨鍙敤鎬ф鏌?
            [[nodiscard]] virtual bool isMemoryAvailable(uint64_t requiredBytes) const = 0;

            // 寮哄埗閲婃斁鏈娇鐢ㄧ殑鍐呭瓨
            virtual void collectGarbage() = 0;

            // 纰庣墖鏁寸悊锛堝鏋滄敮鎸侊級
            virtual void defragment() = 0;
    };

    using IResourceManagerPtr = std::shared_ptr<IResourceManager>;
} // namespace vulkan_engine::vulkan::memory