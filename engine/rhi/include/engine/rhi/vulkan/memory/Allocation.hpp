#pragma once

#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <optional>

namespace vulkan_engine::vulkan::memory
{
    // 鍓嶅悜澹版槑
    class VmaBuffer;
    class VmaImage;

    // VMA 鍒嗛厤淇℃伅
    struct AllocationInfo
    {
        VkDeviceSize size               = 0;
        VkDeviceSize alignment          = 0;
        uint32_t     memoryTypeIndex    = 0;
        void*        mappedData         = nullptr; // 濡傛灉宸叉槧灏?
        bool         isPersistentMapped = false;
    };

    // VMA 鍒嗛厤鐨?RAII 鍖呰鍣?
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

            // 鑾峰彇鍒嗛厤淇℃伅
            AllocationInfo getInfo() const;
            VkDeviceSize   size() const;
            bool           isValid() const noexcept { return allocation_ != VK_NULL_HANDLE; }

            // 鍐呭瓨鏄犲皠锛堜粎瀵?host-visible 鍐呭瓨锛?
            void* map();
            void  unmap();
            bool  isMapped() const noexcept { return mappedData_ != nullptr; }

            // 鍒锋柊/浣块潪鐩稿共鍐呭瓨
            void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
            void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

            // 鑾峰彇鍘熺敓鍒嗛厤鍙ユ焺
            VmaAllocation handle() const noexcept { return allocation_; }

            // 鑾峰彇鍒嗛厤鍣?
            std::shared_ptr<VmaAllocator> allocator() const { return allocator_.lock(); }

        private:
            std::weak_ptr<VmaAllocator> allocator_;
            VmaAllocation               allocation_       = VK_NULL_HANDLE;
            void*                       mappedData_       = nullptr;
            bool                        explicitlyMapped_ = false; // 鍖哄垎鏄惧紡鏄犲皠鍜屾寔涔呮槧灏?

            void cleanup() noexcept;
    };

    using AllocationPtr = std::shared_ptr<Allocation>;

    // 鍒嗛厤鍒涘缓鏍囧織杈呭姪绫?
    class AllocationBuilder
    {
        public:
            AllocationBuilder() = default;

            // 浣跨敤妯″紡
            AllocationBuilder& hostVisible(bool persistentMap = false);
            AllocationBuilder& deviceLocal();
            AllocationBuilder& hostCached();
            AllocationBuilder& sequentialWrite(); // 浼樺寲椤哄簭鍐欏叆
            AllocationBuilder& strategyMinMemory();
            AllocationBuilder& strategyMinTime();
            AllocationBuilder& mapped();

            // 楂樼骇閫夐」
            AllocationBuilder& pool(VmaPool pool);
            AllocationBuilder& userData(void* data);
            AllocationBuilder& priority(float priority); // 0.0 - 1.0

            // 鏋勫缓
            VmaAllocationCreateInfo build() const noexcept { return info_; }

        private:
            VmaAllocationCreateInfo info_{};
            float                   priority_ = 0.5f;
    };
} // namespace vulkan_engine::vulkan::memory