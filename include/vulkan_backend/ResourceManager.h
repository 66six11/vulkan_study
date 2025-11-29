//
// Created by C66 on 2025/11/22.
//
#pragma once
#include <limits>
#include <shared_mutex>
#include <string>
#include <vector>
#include "core/constants.h"
#include "vulkan_backend/VulkanDevice.h"
#include "renderer/Vertex.h"
#include "vulkan_backend/Buffer.h"


#pragma once
#include <limits>
#include <shared_mutex>
#include <string>
#include <vector>

#include "core/constants.h"
#include "vulkan_backend/VulkanDevice.h"
#include "renderer/Vertex.h"

/**
 * @brief 统一管理 Vulkan 缓冲区、图像、采样器和 Mesh 等 GPU 资源
 *
 * ResourceManager 负责封装 VkBuffer / VkImage / VkSampler 等资源的创建与销毁，
 * 提供基于轻量句柄的访问接口，便于资源重用和集中管理生命周期。
 */
class ResourceManager
{
    public:
        // ========================= 句柄类型 =========================

        struct BufferHandle
        {
            uint32_t index{std::numeric_limits<uint32_t>::max()};
            uint32_t generation{0};

            friend bool operator==(const BufferHandle&, const BufferHandle&) = default;
            explicit    operator bool() const noexcept { return index != UINT32_MAX; }
        };

        struct ImageHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
            explicit operator bool() const noexcept { return index != UINT32_MAX; }
        };

        struct SamplerHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
            explicit operator bool() const noexcept { return index != UINT32_MAX; }
        };

        struct MeshHandle
        {
            uint32_t index{UINT32_MAX};
            uint32_t generation{0};
            explicit operator bool() const noexcept { return index != UINT32_MAX; }
        };

        // ========================= 生命周期 =========================

        explicit ResourceManager(VulkanDevice& device);
        ~ResourceManager();

        ResourceManager(const ResourceManager&)            = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // ========================= 描述结构 =========================

        struct BufferDesc
        {
            VkDeviceSize          size;        ///< 缓冲区大小（字节）
            VkBufferUsageFlags    usage;       ///< 指定顶点/索引/统一缓冲等用途
            VkMemoryPropertyFlags memoryFlags; ///< 指定显存/主机可见等属性
            std::string           debugName;   ///< 调试名称
        };

        struct ImageDesc
        {
            VkExtent3D            extent; ///< 图像尺寸（宽、高、深）
            VkFormat              format; ///< 像素格式
            VkImageUsageFlags     usage;  ///< 采样/颜色附件/深度附件等用途
            VkImageAspectFlags    aspect; ///< 视图 aspect 标志（颜色/深度）
            uint32_t              mipLevels{1};
            uint32_t              arrayLayers{1};
            VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
            std::string           debugName; ///< 调试名称
        };

        struct MeshDesc
        {
            uint32_t vertexCount = 0;
            uint32_t indexCount  = 0;
            // 后续可扩展 primitive type、顶点布局 ID 等
        };

        // ========================= Buffer 接口 =========================

        BufferHandle createBuffer(const BufferDesc& desc);
        void         destroyBuffer(BufferHandle handle);

        VkBuffer          getBuffer(BufferHandle handle) const;
        const BufferDesc& getBufferDesc(BufferHandle handle) const;

        void uploadBuffer(
            BufferHandle handle,
            const void*  data,
            VkDeviceSize size,
            VkDeviceSize offset = 0);

        // ========================= Image 接口 =========================

        ImageHandle createImage(const ImageDesc& desc);
        void        destroyImage(ImageHandle handle);

        VkImage          getImage(ImageHandle handle) const;
        VkImageView      getImageView(ImageHandle handle) const;
        const ImageDesc& getImageDesc(ImageHandle handle) const;

        // ========================= Sampler 接口 =========================

        SamplerHandle createSampler(
            const VkSamplerCreateInfo& info,
            std::string_view           debugName = {});
        void destroySampler(SamplerHandle handle);

        VkSampler getSampler(SamplerHandle handle) const;

        // ========================= Mesh 接口 =========================

        MeshHandle createMesh(
            const void* vertexData,
            size_t      vertexCount,
            const void* indexData,
            size_t      indexCount,
            std::string debugName = {});

        void            destroyMesh(MeshHandle mesh);
        const MeshDesc& getMeshDesc(MeshHandle mesh) const;
        BufferHandle    getMeshVertexBuffer(MeshHandle mesh) const;
        BufferHandle    getMeshIndexBuffer(MeshHandle mesh) const;

        // ========================= 其它 =========================

        /// 垃圾回收接口，用于延迟销毁或分帧回收资源（当前实现可为空）
        void garbageCollect();

    private:
        // ========================= 内部存储结构 =========================

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

        struct MeshEntry
        {
            BufferHandle vertexBuffer;
            BufferHandle indexBuffer;
            MeshDesc     desc{};
            uint32_t     generation{0};
            bool         alive{false};
        };

        // ========================= 成员数据 =========================

        VulkanDevice& device_;
        // 资源存储数组
        std::vector<BufferEntry>  buffers_;
        std::vector<ImageEntry>   images_;
        std::vector<SamplerEntry> samplers_;
        std::vector<MeshEntry>    meshes_;
        // 空闲索引列表，用于重用已删除资源的位置
        std::vector<uint32_t> freeBufferIndices_;
        std::vector<uint32_t> freeImageIndices_;
        std::vector<uint32_t> freeSamplerIndices_;
        std::vector<uint32_t> freeMeshIndices_;

        // ========================= 互斥锁 =========================
        mutable std::shared_mutex bufferMutex_;
        mutable std::shared_mutex imageMutex_;
        mutable std::shared_mutex samplerMutex_;
        mutable std::shared_mutex meshMutex_;
};
