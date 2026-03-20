#pragma once

#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/memory/VmaBuffer.hpp"
#include "vulkan/memory/VmaImage.hpp"
#include "vulkan/memory/MemoryPool.hpp"
#include <memory>
#include <unordered_map>
#include <string>

namespace vulkan_engine::vulkan::memory
{
    // 资源管理器 - 统一管理所有 VMA 资源
    class ResourceManager
    {
        public:
            struct CreateInfo
            {
                bool enableDefaultPools    = true;
                bool enableDefragmentation = true;
                bool enableBudget          = true;
            };

            explicit ResourceManager(std::shared_ptr<DeviceManager> deviceManager, const CreateInfo& createInfo = {});
            ~ResourceManager() = default;

            // Non-copyable
            ResourceManager(const ResourceManager&)            = delete;
            ResourceManager& operator=(const ResourceManager&) = delete;

            // 获取 VMA 分配器
            std::shared_ptr<VmaAllocator>  allocator() const { return allocator_; }
            std::shared_ptr<DeviceManager> device() const { return device_; }

            // Buffer 创建便捷方法
            VmaBufferPtr createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo);
            VmaBufferPtr createStagingBuffer(VkDeviceSize size);
            VmaBufferPtr createVertexBuffer(VkDeviceSize size);
            VmaBufferPtr createIndexBuffer(VkDeviceSize size);
            VmaBufferPtr createUniformBuffer(VkDeviceSize size, bool persistentMap = true);
            VmaBufferPtr createStorageBuffer(VkDeviceSize size, bool hostVisible = false);

            // Image 创建便捷方法
            VmaImagePtr createImage(const VkImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo);
            VmaImagePtr createColorAttachment(
                uint32_t              width,
                uint32_t              height,
                VkFormat              format,
                uint32_t              mipLevels = 1,
                VkSampleCountFlagBits samples   = VK_SAMPLE_COUNT_1_BIT);
            VmaImagePtr createDepthAttachment(
                uint32_t              width,
                uint32_t              height,
                VkFormat              format,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
            VmaImagePtr createTexture(
                uint32_t width,
                uint32_t height,
                VkFormat format,
                uint32_t mipLevels   = 1,
                uint32_t arrayLayers = 1);
            VmaImagePtr createCubemap(uint32_t size, VkFormat format, uint32_t mipLevels = 1);

            // 内存池访问
            MemoryPoolManager&       poolManager() { return *poolManager_; }
            const MemoryPoolManager& poolManager() const { return *poolManager_; }

            // 统计信息
            void printStats() const;

            // 获取 JSON 格式的详细统计
            std::string buildStatsString(bool detailed = true) const;

            // 预算查询
            std::vector<VmaBudget> getHeapBudgets() const;
            bool                   isMemoryAvailable(VkDeviceSize requiredBytes) const;

            // 显式资源销毁（通常不需要，RAII 会自动处理）
            void destroyBuffer(VmaBufferPtr buffer);
            void destroyImage(VmaImagePtr image);

            // 强制垃圾回收（释放未使用的内存块）
            void defragment();
            void flush();

        private:
            std::shared_ptr<DeviceManager>     device_;
            std::shared_ptr<VmaAllocator>      allocator_;
            std::unique_ptr<MemoryPoolManager> poolManager_;

            // 追踪所有资源（用于调试和统计）
            std::unordered_map<VmaBuffer*, VmaBufferPtr> buffers_;
            std::unordered_map<VmaImage*, VmaImagePtr>   images_;
    };

    using ResourceManagerPtr = std::shared_ptr<ResourceManager>;
} // namespace vulkan_engine::vulkan::memory