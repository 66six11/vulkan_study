//
// Created by C66 on 2025/11/22.
//
#pragma once
#include "constants.h"
#include "VulkanDevice.h"
#include <shared_mutex>

struct BufferHandle
{
    uint32_t index{std::numeric_limits<uint32_t>::max()};
    uint32_t generation{0};

    friend bool operator==(const BufferHandle&, const BufferHandle&) = default;
    explicit    operator bool() const noexcept { return index != UINT32_MAX; }
};

class ResourceManager
{
    public:
        struct ImageHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
        };

        struct SamplerHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
        };

        explicit ResourceManager(VulkanDevice& device);
        ~ResourceManager();

        ResourceManager(const ResourceManager&)            = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        struct BufferDesc
        {
            VkDeviceSize          size;
            VkBufferUsageFlags    usage;
            VkMemoryPropertyFlags memoryFlags;
            std::string           debugName;
        };

        struct ImageDesc
        {
            VkExtent3D            extent;
            VkFormat              format;
            VkImageUsageFlags     usage;
            VkImageAspectFlags    aspect;
            uint32_t              mipLevels{1};
            uint32_t              arrayLayers{1};
            VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
            std::string           debugName;
        };

        // 创建/销毁
        BufferHandle createBuffer(const BufferDesc& desc);
        void         destroyBuffer(BufferHandle handle);

        ImageHandle createImage(const ImageDesc& desc);
        void        destroyImage(ImageHandle handle);

        SamplerHandle createSampler(const VkSamplerCreateInfo& info,
                                    std::string_view           debugName = {});
        void destroySampler(SamplerHandle handle);

        // 访问底层 Vk 对象
        VkBuffer    getBuffer(BufferHandle handle) const;
        VkImage     getImage(ImageHandle handle) const;
        VkImageView getImageView(ImageHandle handle) const; // 如果内部帮你建 view
        VkSampler   getSampler(SamplerHandle handle) const;

        // 资源元数据（format, size 等）
        const BufferDesc& getBufferDesc(BufferHandle handle) const;
        const ImageDesc&  getImageDesc(ImageHandle handle) const;

        // 更新/上传：可选
        void uploadBuffer(BufferHandle handle,
                          const void*  data,
                          VkDeviceSize size,
                          VkDeviceSize offset = 0);

        // 池化/回收接口（可内部自用）
        void garbageCollect(); // 延迟销毁、frame-based 回收等

    private:
        struct BufferEntry
        {
            VkBuffer       buffer{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE}; // 若没用 VMA
            BufferDesc     desc{};
            uint32_t       generation{0};
            bool           alive{false};
        };

        struct ImageEntry
        {
            VkImage        image{VK_NULL_HANDLE};
            VkDeviceMemory memory{VK_NULL_HANDLE};
            VkImageView    defaultView{VK_NULL_HANDLE};
            ImageDesc      desc{};
            uint32_t       generation{0};
            bool           alive{false};
        };

        struct SamplerEntry
        {
            VkSampler   sampler{VK_NULL_HANDLE};
            std::string name;
            uint32_t    generation{0};
            bool        alive{false};
        };

        VulkanDevice& device_;

        std::vector<BufferEntry>  buffers_;
        std::vector<ImageEntry>   images_;
        std::vector<SamplerEntry> samplers_;
        std::vector<uint32_t>     freeBufferIndices_;
        std::vector<uint32_t>     freeImageIndices_;
        std::vector<uint32_t>     freeSamplerIndices_;

        // 可选细粒度锁：如果要支持多线程创建资源
        mutable std::shared_mutex bufferMutex_;
        mutable std::shared_mutex imageMutex_;
        mutable std::shared_mutex samplerMutex_;

        // 内部辅助函数：分配 index、创建 Vk 资源、绑定内存等
};

