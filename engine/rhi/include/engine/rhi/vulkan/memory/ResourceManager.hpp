#pragma once

#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include "engine/rhi/vulkan/memory/VmaBuffer.hpp"
#include "engine/rhi/vulkan/memory/VmaImage.hpp"
#include "engine/rhi/vulkan/memory/MemoryPool.hpp"
#include <memory>
#include <unordered_map>
#include <string>

namespace vulkan_engine::vulkan::memory
{
    // 璧勬簮绠＄悊鍣?- 缁熶竴绠＄悊鎵€鏈?VMA 璧勬簮
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

            // 鑾峰彇 VMA 鍒嗛厤鍣?
            std::shared_ptr<VmaAllocator>  allocator() const { return allocator_; }
            std::shared_ptr<DeviceManager> device() const { return device_; }

            // Buffer 鍒涘缓渚挎嵎鏂规硶
            VmaBufferPtr createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo);
            VmaBufferPtr createStagingBuffer(VkDeviceSize size);
            VmaBufferPtr createVertexBuffer(VkDeviceSize size);
            VmaBufferPtr createIndexBuffer(VkDeviceSize size);
            VmaBufferPtr createUniformBuffer(VkDeviceSize size, bool persistentMap = true);
            VmaBufferPtr createStorageBuffer(VkDeviceSize size, bool hostVisible = false);

            // Image 鍒涘缓渚挎嵎鏂规硶
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

            // 鍐呭瓨姹犺闂?
            MemoryPoolManager&       poolManager() { return *poolManager_; }
            const MemoryPoolManager& poolManager() const { return *poolManager_; }

            // 缁熻淇℃伅
            void printStats() const;

            // 鑾峰彇 JSON 鏍煎紡鐨勮缁嗙粺璁?
            std::string buildStatsString(bool detailed = true) const;

            // 棰勭畻鏌ヨ
            std::vector<VmaBudget> getHeapBudgets() const;
            bool                   isMemoryAvailable(VkDeviceSize requiredBytes) const;

            // 鏄惧紡璧勬簮閿€姣侊紙閫氬父涓嶉渶瑕侊紝RAII 浼氳嚜鍔ㄥ鐞嗭級
            void destroyBuffer(VmaBufferPtr buffer);
            void destroyImage(VmaImagePtr image);

            // 寮哄埗鍨冨溇鍥炴敹锛堥噴鏀炬湭浣跨敤鐨勫唴瀛樺潡锛?
            void defragment();
            void flush();

        private:
            std::shared_ptr<DeviceManager>     device_;
            std::shared_ptr<VmaAllocator>      allocator_;
            std::unique_ptr<MemoryPoolManager> poolManager_;

            // 杩借釜鎵€鏈夎祫婧愶紙鐢ㄤ簬璋冭瘯鍜岀粺璁★級
            std::unordered_map<VmaBuffer*, VmaBufferPtr> buffers_;
            std::unordered_map<VmaImage*, VmaImagePtr>   images_;
    };

    using ResourceManagerPtr = std::shared_ptr<ResourceManager>;
} // namespace vulkan_engine::vulkan::memory