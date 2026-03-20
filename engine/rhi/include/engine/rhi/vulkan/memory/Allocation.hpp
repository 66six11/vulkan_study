#pragma once

#include "vulkan/memory/VmaAllocator.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // 前向声明
    class VmaBuffer;
    class VmaImage;

    // VMA 分配信息
    struct AllocationInfo
    {
        VkDeviceSize size               = 0;
        VkDeviceSize alignment          = 0;
        uint32_t     memoryTypeIndex    = 0;
        void*        mappedData         = nullptr; // 如果已映射
        bool         isPersistentMapped = false;
    };

    // VMA 分配的 RAII 包装器
    class Allocation
    {
        public:
            Allocation() noexcept = default;
            Allocation(std::shared_ptr<VmaAllocator> allocator, VmaAllocation allocation) noexcept;
            ~Allocation();

            // Non-copyable
            Allocation(const Allocation&)            = delete;
            Allocation& operator=(const Allocation&) = delete;

            // Movable
            Allocation(Allocation&& other) noexcept;
            Allocation& operator=(Allocation&& other) noexcept;

            // 获取分配信息
            AllocationInfo getInfo() const;
            VkDeviceSize   size() const;
            bool           isValid() const noexcept { return allocation_ != VK_NULL_HANDLE; }

            // 内存映射（仅对 host-visible 内存）
            void* map();
            void  unmap();
            bool  isMapped() const noexcept { return mappedData_ != nullptr; }

            // 刷新/使非相干内存
            void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
            void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

            // 获取原生分配句柄
            VmaAllocation handle() const noexcept { return allocation_; }

            // 获取分配器
            std::shared_ptr<VmaAllocator> allocator() const { return allocator_.lock(); }

        private:
            std::weak_ptr<VmaAllocator> allocator_;
            VmaAllocation               allocation_       = VK_NULL_HANDLE;
            void*                       mappedData_       = nullptr;
            bool                        explicitlyMapped_ = false; // 区分显式映射和持久映射

            void cleanup() noexcept;
    };

    using AllocationPtr = std::shared_ptr<Allocation>;

    // 分配创建标志辅助类
    class AllocationBuilder
    {
        public:
            AllocationBuilder() = default;

            // 使用模式
            AllocationBuilder& hostVisible(bool persistentMap = false);
            AllocationBuilder& deviceLocal();
            AllocationBuilder& hostCached();
            AllocationBuilder& sequentialWrite(); // 优化顺序写入
            AllocationBuilder& strategyMinMemory();
            AllocationBuilder& strategyMinTime();
            AllocationBuilder& mapped();

            // 高级选项
            AllocationBuilder& pool(VmaPool pool);
            AllocationBuilder& userData(void* data);
            AllocationBuilder& priority(float priority); // 0.0 - 1.0

            // 构建
            VmaAllocationCreateInfo build() const noexcept { return info_; }

        private:
            VmaAllocationCreateInfo info_{};
            float                   priority_ = 0.5f;
    };
} // namespace vulkan_engine::vulkan::memory