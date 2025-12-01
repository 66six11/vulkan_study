// ResourceManager.h
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

#include "core/constants.h"   // 假定已包含 vulkan_core.h
#include "vulkan_backend/VulkanDevice.h"


namespace vkresource
{
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

    // ========================= 描述结构 =========================

    struct BufferDesc
    {
        VkDeviceSize          size{};        ///< 缓冲区大小（字节）
        VkBufferUsageFlags    usage{};       ///< 用途标志（顶点/索引/统一缓冲等）
        VkMemoryPropertyFlags memoryFlags{}; ///< 内存属性（HOST_VISIBLE / DEVICE_LOCAL 等）
        std::string           debugName;     ///< 调试名称
    };

    struct ImageDesc
    {
        VkExtent3D            extent{}; ///< 宽/高/深
        VkFormat              format{VK_FORMAT_UNDEFINED};
        VkImageUsageFlags     usage{};  ///< 采样/颜色附件/深度附件等
        VkImageAspectFlags    aspect{}; ///< 颜色/深度等
        uint32_t              mipLevels{1};
        uint32_t              arrayLayers{1};
        VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
        std::string           debugName;
    };

    struct MeshDesc
    {
        VkDeviceSize          vertexBufferSize{0};  ///< 顶点缓冲大小（字节）
        VkDeviceSize          indexBufferSize{0};   ///< 索引缓冲大小（字节，可为 0）
        VkBufferUsageFlags    vertexUsage{0};       ///< 一般至少包含 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        VkBufferUsageFlags    indexUsage{0};        ///< 一般至少包含 VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        VkMemoryPropertyFlags vertexMemoryFlags{0}; ///< 顶点缓冲内存属性
        VkMemoryPropertyFlags indexMemoryFlags{0};  ///< 索引缓冲内存属性
        std::string           vertexDebugName;
        std::string           indexDebugName;
        // 后续可扩展：顶点格式 ID / primitive 类型等
    };


    /// 统一管理 Vulkan Buffer / Image / Sampler / Mesh 等 GPU 资源
    /// - 提供简单的句柄（index + generation）
    /// - 每个资源独立 VkDeviceMemory（适合教学/小项目，后续可替换为 VMA 等分配器）
    class ResourceManager
    {
        public:
            // ========================= 生命周期 =========================

            explicit ResourceManager(VulkanDevice& device) noexcept;
            ~ResourceManager() noexcept;

            ResourceManager(const ResourceManager&)            = delete;
            ResourceManager& operator=(const ResourceManager&) = delete;

            // ========================= Buffer 接口 =========================

            BufferHandle createBuffer(const BufferDesc& desc);
            void         destroyBuffer(BufferHandle handle) noexcept;

            VkBuffer          getBuffer(BufferHandle handle) const noexcept;
            const BufferDesc& getBufferDesc(BufferHandle handle) const; ///< 非法句柄抛异常

            /// 直接映射上传数据，仅允许 HOST_VISIBLE 缓冲
            void uploadBuffer(
                BufferHandle handle,
                const void*  data,
                VkDeviceSize size,
                VkDeviceSize offset = 0);

            // ========================= Image 接口 =========================

            ImageHandle createImage(const ImageDesc& desc);
            void        destroyImage(ImageHandle handle) noexcept;

            VkImage          getImage(ImageHandle handle) const noexcept;
            const ImageDesc& getImageDesc(ImageHandle handle) const; ///< 非法句柄抛异常

            // ========================= Sampler 接口 =========================

            SamplerHandle createSampler(
                const VkSamplerCreateInfo& info,
                std::string_view           debugName = {});
            void destroySampler(SamplerHandle handle) noexcept;

            VkSampler getSampler(SamplerHandle handle) const noexcept;

            // ========================= Mesh 接口 =========================
            /// 仅创建顶点/索引缓冲，不做数据上传；上传由外层通过 BufferHandle + uploadBuffer 完成
            MeshHandle createMesh(const MeshDesc& desc);
            void       destroyMesh(MeshHandle handle) noexcept;

            BufferHandle    getMeshVertexBuffer(MeshHandle handle) const; ///< 非法句柄抛异常
            BufferHandle    getMeshIndexBuffer(MeshHandle handle) const;  ///< 非法或无索引缓冲则返回无效句柄
            const MeshDesc& getMeshDesc(MeshHandle handle) const;         ///< 非法句柄抛异常

            // ========================= 其它 =========================

            /// 预留：将来做延迟销毁/分帧回收
            void garbageCollect() noexcept
            {
            }

        private:
            // ========================= 内部条目结构 =========================

            struct BufferEntry
            {
                VkBuffer       buffer{VK_NULL_HANDLE};
                VkDeviceMemory memory{VK_NULL_HANDLE};
                BufferDesc     desc{};
                uint32_t       generation{0};
                bool           alive{false};
            };

            struct ImageEntry
            {
                VkImage        image{VK_NULL_HANDLE};
                VkDeviceMemory memory{VK_NULL_HANDLE};
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
                BufferHandle vertexBuffer{};
                BufferHandle indexBuffer{};
                MeshDesc     desc{};
                uint32_t     generation{0};
                bool         alive{false};
            };

        private:
            VulkanDevice& device_;

            std::vector<BufferEntry>  buffers_;
            std::vector<ImageEntry>   images_;
            std::vector<SamplerEntry> samplers_;
            std::vector<MeshEntry>    meshes_;

            std::vector<uint32_t> freeBufferIndices_;
            std::vector<uint32_t> freeImageIndices_;
            std::vector<uint32_t> freeSamplerIndices_;
            std::vector<uint32_t> freeMeshIndices_;

        private:
            // 内部工具函数

            uint32_t allocateBufferSlot();
            uint32_t allocateImageSlot();
            uint32_t allocateSamplerSlot();
            uint32_t allocateMeshSlot();

            void destroyBufferInternal(uint32_t index) noexcept;
            void destroyImageInternal(uint32_t index) noexcept;
            void destroySamplerInternal(uint32_t index) noexcept;

            uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

            const BufferEntry* tryGetBufferEntry(BufferHandle handle) const noexcept;
            BufferEntry*       tryGetBufferEntry(BufferHandle handle) noexcept;

            const ImageEntry* tryGetImageEntry(ImageHandle handle) const noexcept;

            const SamplerEntry* tryGetSamplerEntry(SamplerHandle handle) const noexcept;

            const MeshEntry* tryGetMeshEntry(MeshHandle handle) const noexcept;
            MeshEntry*       tryGetMeshEntry(MeshHandle handle) noexcept;

            const BufferEntry& getValidBufferEntry(BufferHandle handle) const;
            BufferEntry&       getValidBufferEntry(BufferHandle handle);

            const MeshEntry& getValidMeshEntry(MeshHandle handle) const;
            MeshEntry&       getValidMeshEntry(MeshHandle handle);

            void setDebugName(VkObjectType type, uint64_t handle, std::string_view name) const noexcept;
    };
}
