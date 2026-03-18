#pragma once

#include "vulkan/device/Device.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <string>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // VMA 统计信息
    struct VmaStats
    {
        uint64_t totalBytesAllocated = 0;
        uint64_t totalBytesUsed      = 0;
        uint32_t allocationCount     = 0;
        uint32_t poolCount           = 0;
        uint32_t blockCount          = 0;
    };

    // VMA 池创建信息
    struct PoolCreateInfo
    {
        uint32_t     memoryTypeIndex = 0;
        VkDeviceSize blockSize       = 0; // 0 = 使用默认
        uint32_t     minBlockCount   = 0;
        uint32_t     maxBlockCount   = 0;
        std::string  name;
    };

    // VMA 分配器管理器
    class VmaAllocator
    {
        public:
            struct CreateInfo
            {
                bool enableDefragmentation     = true;
                bool enableBudget              = true;  // 启用预算查询
                bool recordAllocations         = false; // Debug build 时启用
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

            // 获取原生 VMA 分配器
            ::VmaAllocator handle() const noexcept { return allocator_; }
            bool           isValid() const noexcept { return allocator_ != VK_NULL_HANDLE; }

            // 设备访问
            std::shared_ptr<DeviceManager> device() const { return deviceManager_; }

            // 内存池管理
            VmaPool createPool(const PoolCreateInfo& info);
            void    destroyPool(VmaPool pool);

            // 统计信息
            VmaStats getStats() const;
            void     printStats() const;

            // 预算查询（GPU 内存预算）
            struct Budget
            {
                VkDeviceSize budgetBytes = 0;
                VkDeviceSize usageBytes  = 0;
            };

            std::vector<Budget> getHeapBudgets() const;

            // 检查是否支持特定内存类型
            bool supportsMemoryType(uint32_t memoryTypeIndex, VkMemoryPropertyFlags requiredFlags) const;

            // 工具函数
            static std::string allocationFlagsToString(VmaAllocationCreateFlags flags);

        private:
            std::shared_ptr<DeviceManager> deviceManager_;
            ::VmaAllocator                 allocator_ = VK_NULL_HANDLE;
            std::vector<VmaPool>           pools_;

            void cleanup() noexcept;
    };

    using VmaAllocatorPtr = std::shared_ptr<VmaAllocator>;
} // namespace vulkan_engine::vulkan::memory
