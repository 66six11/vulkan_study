#pragma once

#include "engine/rhi/vulkan/device/Device.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // 浣跨敤 VMA 鍘熺敓鐨?VmaBudget 鍜?VmaStats

    // VMA 姹犲垱寤轰俊鎭?
    struct PoolCreateInfo
    {
        uint32_t     memoryTypeIndex = 0;
        VkDeviceSize blockSize       = 0; // 0 = 浣跨敤榛樿
        uint32_t     minBlockCount   = 0;
        uint32_t     maxBlockCount   = 0;
        std::string  name;
    };

    // VMA 鍒嗛厤鍣ㄧ鐞嗗櫒
    class VmaAllocator
    {
        public:
            struct CreateInfo
            {
                bool enableDefragmentation     = true;
                bool enableBudget              = true;  // 鍚敤棰勭畻鏌ヨ
                bool recordAllocations         = false; // Debug build 鏃跺惎鐢?
                bool enableMemoryLeakDetection = false;
            };

            VmaAllocator(std::shared_ptr<DeviceManager> deviceManager, const CreateInfo& createInfo = {});
            ~VmaAllocator();

            // Non-copyable
            VmaAllocator(const VmaAllocator&)            = delete;
            VmaAllocator& operator=(const VmaAllocator&) = delete;

            // Movable
            VmaAllocator(VmaAllocator&& other) noexcept;
            VmaAllocator& operator=(VmaAllocator&& other) noexcept;

            // 鑾峰彇鍘熺敓 VMA 鍒嗛厤鍣?
            ::VmaAllocator handle() const noexcept { return allocator_; }
            bool           isValid() const noexcept { return allocator_ != VK_NULL_HANDLE; }

            // 璁惧璁块棶
            std::shared_ptr<DeviceManager> device() const { return deviceManager_; }

            // 鍐呭瓨姹犵鐞?
            VmaPool createPool(const PoolCreateInfo& info);
            void    destroyPool(VmaPool pool);

            // 缁熻淇℃伅 - 鐩存帴浣跨敤 VMA 鍘熺敓鍔熻兘
            // 鎵撳嵃缁熻淇℃伅鍒版棩蹇?
            void printStats() const;

            // 鑾峰彇 JSON 鏍煎紡鐨勮缁嗙粺璁★紙閫傚悎璋冭瘯锛?
            std::string buildStatsString(bool detailed = true) const;

            // 棰勭畻鏌ヨ锛圙PU 鍐呭瓨棰勭畻锛? 杩斿洖鍫嗛绠楁暟缁?
            std::vector<VmaBudget> getHeapBudgets() const;

            // 瀵煎嚭璇︾粏鍒嗛厤淇℃伅鍒版棩蹇?
            void dumpAllocations() const;

            // 妫€鏌ユ槸鍚︽敮鎸佺壒瀹氬唴瀛樼被鍨?
            bool supportsMemoryType(uint32_t memoryTypeIndex, VkMemoryPropertyFlags requiredFlags) const;

            // 宸ュ叿鍑芥暟
            static std::string allocationFlagsToString(VmaAllocationCreateFlags flags);

        private:
            std::shared_ptr<DeviceManager> deviceManager_;
            ::VmaAllocator                 allocator_ = VK_NULL_HANDLE;
            std::vector<VmaPool>           pools_;

            void cleanup() noexcept;
    };

    using VmaAllocatorPtr = std::shared_ptr<VmaAllocator>;
} // namespace vulkan_engine::vulkan::memory