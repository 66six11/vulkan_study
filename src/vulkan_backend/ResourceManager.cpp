#include "vulkan_backend/ResourceManager.h"

#include <cstring>   // std::memcpy

namespace
{
    /// 简单封装：查找满足 flags 的内存类型索引
    uint32_t findMemoryTypeImpl(
        uint32_t                                typeFilter,
        VkMemoryPropertyFlags                   properties,
        const VkPhysicalDeviceMemoryProperties& memProps)
    {
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type.");
    }
} // namespace

// ========================= 构造 / 析构 =========================
namespace vkresource
{
    ResourceManager::ResourceManager(VulkanDevice& device) noexcept
        : device_(device)
    {
    }

    ResourceManager::~ResourceManager() noexcept
    {
        // 注意：这里只做“尽力释放”，假定此时 GPU 不再使用这些资源
        for (uint32_t i = 0; i < meshes_.size(); ++i)
        {
            MeshEntry& me = meshes_[i];
            me.alive      = false; // Mesh 只持有 BufferHandle，本身没有 Vulkan 资源
        }

        for (uint32_t i = 0; i < samplers_.size(); ++i)
        {
            destroySamplerInternal(i);
        }
        for (uint32_t i = 0; i < images_.size(); ++i)
        {
            destroyImageInternal(i);
        }
        for (uint32_t i = 0; i < buffers_.size(); ++i)
        {
            destroyBufferInternal(i);
        }
    }

    // ========================= slot 分配 =========================

    uint32_t ResourceManager::allocateBufferSlot()
    {
        if (!freeBufferIndices_.empty())
        {
            uint32_t idx = freeBufferIndices_.back();
            freeBufferIndices_.pop_back();
            return idx;
        }
        buffers_.emplace_back();
        return static_cast<uint32_t>(buffers_.size() - 1);
    }

    uint32_t ResourceManager::allocateImageSlot()
    {
        if (!freeImageIndices_.empty())
        {
            uint32_t idx = freeImageIndices_.back();
            freeImageIndices_.pop_back();
            return idx;
        }
        images_.emplace_back();
        return static_cast<uint32_t>(images_.size() - 1);
    }

    uint32_t ResourceManager::allocateSamplerSlot()
    {
        if (!freeSamplerIndices_.empty())
        {
            uint32_t idx = freeSamplerIndices_.back();
            freeSamplerIndices_.pop_back();
            return idx;
        }
        samplers_.emplace_back();
        return static_cast<uint32_t>(samplers_.size() - 1);
    }

    uint32_t ResourceManager::allocateMeshSlot()
    {
        if (!freeMeshIndices_.empty())
        {
            uint32_t idx = freeMeshIndices_.back();
            freeMeshIndices_.pop_back();
            return idx;
        }
        meshes_.emplace_back();
        return static_cast<uint32_t>(meshes_.size() - 1);
    }

    // ========================= 内部销毁 =========================

    void ResourceManager::destroyBufferInternal(uint32_t index) noexcept
    {
        if (index >= buffers_.size())
            return;

        BufferEntry& e = buffers_[index];
        if (!e.alive)
            return;

        VkDevice dev = device_.device();
        if (e.buffer)
        {
            vkDestroyBuffer(dev, e.buffer, nullptr);
            e.buffer = VK_NULL_HANDLE;
        }
        if (e.memory)
        {
            vkFreeMemory(dev, e.memory, nullptr);
            e.memory = VK_NULL_HANDLE;
        }
        e.alive = false;
        // generation 不清零，保持单调递增属性
    }

    void ResourceManager::destroyImageInternal(uint32_t index) noexcept
    {
        if (index >= images_.size())
            return;

        ImageEntry& e = images_[index];
        if (!e.alive)
            return;

        VkDevice dev = device_.device();
        if (e.image)
        {
            vkDestroyImage(dev, e.image, nullptr);
            e.image = VK_NULL_HANDLE;
        }
        if (e.memory)
        {
            vkFreeMemory(dev, e.memory, nullptr);
            e.memory = VK_NULL_HANDLE;
        }
        e.alive = false;
    }

    void ResourceManager::destroySamplerInternal(uint32_t index) noexcept
    {
        if (index >= samplers_.size())
            return;

        SamplerEntry& e = samplers_[index];
        if (!e.alive)
            return;

        VkDevice dev = device_.device();
        if (e.sampler)
        {
            vkDestroySampler(dev, e.sampler, nullptr);
            e.sampler = VK_NULL_HANDLE;
        }
        e.alive = false;
    }

    // ========================= 工具：内存类型 / 句柄检查 =========================

    uint32_t ResourceManager::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const
    {
        return findMemoryTypeImpl(typeBits, properties, device_.memoryProperties());
    }

    const ResourceManager::BufferEntry* ResourceManager::tryGetBufferEntry(BufferHandle handle) const noexcept
    {
        if (!handle || handle.index >= buffers_.size())
            return nullptr;

        const BufferEntry& e = buffers_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return nullptr;

        return &e;
    }

    ResourceManager::BufferEntry* ResourceManager::tryGetBufferEntry(BufferHandle handle) noexcept
    {
        return const_cast<BufferEntry*>(std::as_const(*this).tryGetBufferEntry(handle));
    }

    const ResourceManager::ImageEntry* ResourceManager::tryGetImageEntry(ImageHandle handle) const noexcept
    {
        if (!handle || handle.index >= images_.size())
            return nullptr;

        const ImageEntry& e = images_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return nullptr;

        return &e;
    }

    const ResourceManager::SamplerEntry* ResourceManager::tryGetSamplerEntry(SamplerHandle handle) const noexcept
    {
        if (!handle || handle.index >= samplers_.size())
            return nullptr;

        const SamplerEntry& e = samplers_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return nullptr;

        return &e;
    }

    const ResourceManager::MeshEntry* ResourceManager::tryGetMeshEntry(MeshHandle handle) const noexcept
    {
        if (!handle || handle.index >= meshes_.size())
            return nullptr;

        const MeshEntry& e = meshes_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return nullptr;

        return &e;
    }

    ResourceManager::MeshEntry* ResourceManager::tryGetMeshEntry(MeshHandle handle) noexcept
    {
        return const_cast<MeshEntry*>(std::as_const(*this).tryGetMeshEntry(handle));
    }

    const ResourceManager::BufferEntry& ResourceManager::getValidBufferEntry(BufferHandle handle) const
    {
        const BufferEntry* e = tryGetBufferEntry(handle);
        if (!e)
            throw std::runtime_error("Invalid BufferHandle: out of range / not alive / generation mismatch.");
        return *e;
    }

    ResourceManager::BufferEntry& ResourceManager::getValidBufferEntry(BufferHandle handle)
    {
        return const_cast<BufferEntry&>(std::as_const(*this).getValidBufferEntry(handle));
    }

    const ResourceManager::MeshEntry& ResourceManager::getValidMeshEntry(MeshHandle handle) const
    {
        const MeshEntry* e = tryGetMeshEntry(handle);
        if (!e)
            throw std::runtime_error("Invalid MeshHandle: out of range / not alive / generation mismatch.");
        return *e;
    }

    ResourceManager::MeshEntry& ResourceManager::getValidMeshEntry(MeshHandle handle)
    {
        return const_cast<MeshEntry&>(std::as_const(*this).getValidMeshEntry(handle));
    }

    // ========================= Debug 名称（可选） =========================

    void ResourceManager::setDebugName(VkObjectType type, uint64_t handle, std::string_view name) const noexcept
    {
        if (name.empty())
            return;

        #ifdef VK_EXT_debug_utils
        VkDevice dev = device_.device();
        auto     fn  = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
            vkGetDeviceProcAddr(dev, "vkSetDebugUtilsObjectNameEXT"));
        if (!fn)
            return;

        VkDebugUtilsObjectNameInfoEXT info{};
        info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType   = type;
        info.objectHandle = handle;
        info.pObjectName  = name.data();

        fn(dev, &info);
        #else
        (void)type;
        (void)handle;
        (void)name;
        #endif
    }

    // ========================= Buffer 接口实现 =========================

    BufferHandle ResourceManager::createBuffer(const BufferDesc& desc)
    {
        if (desc.size == 0 || desc.usage == 0)
            throw std::runtime_error("BufferDesc invalid: size or usage is zero.");

        uint32_t     index = allocateBufferSlot();
        BufferEntry& e     = buffers_[index];

        if (e.alive)
            destroyBufferInternal(index);

        VkDevice dev = device_.device();

        VkBufferCreateInfo info{};
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size        = desc.size;
        info.usage       = desc.usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(dev, &info, nullptr, &e.buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VkBuffer.");

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(dev, e.buffer, &memReq);

        VkMemoryAllocateInfo alloc{};
        alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize  = memReq.size;
        alloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, desc.memoryFlags);

        if (vkAllocateMemory(dev, &alloc, nullptr, &e.memory) != VK_SUCCESS)
        {
            vkDestroyBuffer(dev, e.buffer, nullptr);
            e.buffer = VK_NULL_HANDLE;
            throw std::runtime_error("Failed to allocate memory for buffer.");
        }

        if (vkBindBufferMemory(dev, e.buffer, e.memory, 0) != VK_SUCCESS)
        {
            vkFreeMemory(dev, e.memory, nullptr);
            vkDestroyBuffer(dev, e.buffer, nullptr);
            e.memory = VK_NULL_HANDLE;
            e.buffer = VK_NULL_HANDLE;
            throw std::runtime_error("Failed to bind memory for buffer.");
        }

        e.desc  = desc;
        e.alive = true;
        ++e.generation; // generation 只递增不清零

        if (!desc.debugName.empty())
        {
            setDebugName(VK_OBJECT_TYPE_BUFFER,
                         reinterpret_cast<uint64_t>(e.buffer),
                         desc.debugName);
        }

        return BufferHandle{index, e.generation};
    }

    void ResourceManager::destroyBuffer(BufferHandle handle) noexcept
    {
        if (!handle || handle.index >= buffers_.size())
            return;

        BufferEntry& e = buffers_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return;

        destroyBufferInternal(handle.index);
        freeBufferIndices_.push_back(handle.index);
    }

    VkBuffer ResourceManager::getBuffer(BufferHandle handle) const noexcept
    {
        const BufferEntry* e = tryGetBufferEntry(handle);
        return e ? e->buffer : VK_NULL_HANDLE;
    }

    const BufferDesc& ResourceManager::getBufferDesc(BufferHandle handle) const
    {
        return getValidBufferEntry(handle).desc;
    }

    void ResourceManager::uploadBuffer(
        BufferHandle handle,
        const void*  data,
        VkDeviceSize size,
        VkDeviceSize offset)
    {
        if (!data || size == 0)
            return;

        BufferEntry& e = getValidBufferEntry(handle);

        // 根据创建时的 memoryFlags 判断是否为 HOST_VISIBLE
        if ((e.desc.memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
            throw std::runtime_error("uploadBuffer requires HOST_VISIBLE memory; got device-local only buffer.");

        VkDevice dev = device_.device();

        void* mapped = nullptr;
        if (vkMapMemory(dev, e.memory, offset, size, 0, &mapped) != VK_SUCCESS)
            throw std::runtime_error("Failed to map buffer memory in uploadBuffer.");

        std::memcpy(mapped, data, static_cast<size_t>(size));

        vkUnmapMemory(dev, e.memory);
    }

    // ========================= Image 接口实现 =========================

    ImageHandle ResourceManager::createImage(const ImageDesc& desc)
    {
        if (desc.extent.width == 0 || desc.extent.height == 0 || desc.format == VK_FORMAT_UNDEFINED)
            throw std::runtime_error("ImageDesc invalid: extent or format.");

        uint32_t    index = allocateImageSlot();
        ImageEntry& e     = images_[index];

        if (e.alive)
            destroyImageInternal(index);

        VkDevice dev = device_.device();

        VkImageCreateInfo info{};
        info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType     = VK_IMAGE_TYPE_2D;
        info.extent        = desc.extent;
        info.mipLevels     = desc.mipLevels;
        info.arrayLayers   = desc.arrayLayers;
        info.format        = desc.format;
        info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage         = desc.usage;
        info.samples       = desc.samples;
        info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(dev, &info, nullptr, &e.image) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VkImage.");

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(dev, e.image, &memReq);

        VkMemoryAllocateInfo alloc{};
        alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize  = memReq.size;
        alloc.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(dev, &alloc, nullptr, &e.memory) != VK_SUCCESS)
        {
            vkDestroyImage(dev, e.image, nullptr);
            e.image = VK_NULL_HANDLE;
            throw std::runtime_error("Failed to allocate memory for image.");
        }

        if (vkBindImageMemory(dev, e.image, e.memory, 0) != VK_SUCCESS)
        {
            vkFreeMemory(dev, e.memory, nullptr);
            vkDestroyImage(dev, e.image, nullptr);
            e.memory = VK_NULL_HANDLE;
            e.image  = VK_NULL_HANDLE;
            throw std::runtime_error("Failed to bind memory for image.");
        }

        e.desc  = desc;
        e.alive = true;
        ++e.generation;

        if (!desc.debugName.empty())
        {
            setDebugName(VK_OBJECT_TYPE_IMAGE,
                         reinterpret_cast<uint64_t>(e.image),
                         desc.debugName);
        }

        return ImageHandle{index, e.generation};
    }

    void ResourceManager::destroyImage(ImageHandle handle) noexcept
    {
        if (!handle || handle.index >= images_.size())
            return;

        ImageEntry& e = images_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return;

        destroyImageInternal(handle.index);
        freeImageIndices_.push_back(handle.index);
    }

    VkImage ResourceManager::getImage(ImageHandle handle) const noexcept
    {
        const ImageEntry* e = tryGetImageEntry(handle);
        return e ? e->image : VK_NULL_HANDLE;
    }

    const ImageDesc& ResourceManager::getImageDesc(ImageHandle handle) const
    {
        const ImageEntry* e = tryGetImageEntry(handle);
        if (!e)
            throw std::runtime_error("Invalid ImageHandle: out of range / not alive / generation mismatch.");
        return e->desc;
    }

    // ========================= Sampler 接口实现 =========================

    SamplerHandle ResourceManager::createSampler(
        const VkSamplerCreateInfo& info,
        std::string_view           debugName)
    {
        uint32_t      index = allocateSamplerSlot();
        SamplerEntry& e     = samplers_[index];

        if (e.alive)
            destroySamplerInternal(index);

        VkDevice dev = device_.device();

        if (vkCreateSampler(dev, &info, nullptr, &e.sampler) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VkSampler.");

        e.name  = std::string(debugName);
        e.alive = true;
        ++e.generation;

        if (!e.name.empty())
        {
            setDebugName(VK_OBJECT_TYPE_SAMPLER,
                         reinterpret_cast<uint64_t>(e.sampler),
                         e.name);
        }

        return SamplerHandle{index, e.generation};
    }

    void ResourceManager::destroySampler(SamplerHandle handle) noexcept
    {
        if (!handle || handle.index >= samplers_.size())
            return;

        SamplerEntry& e = samplers_[handle.index];
        if (!e.alive || e.generation != handle.generation)
            return;

        destroySamplerInternal(handle.index);
        freeSamplerIndices_.push_back(handle.index);
    }

    VkSampler ResourceManager::getSampler(SamplerHandle handle) const noexcept
    {
        const SamplerEntry* e = tryGetSamplerEntry(handle);
        return e ? e->sampler : VK_NULL_HANDLE;
    }

    // ========================= Mesh 接口实现 =========================

    MeshHandle ResourceManager::createMesh(const MeshDesc& desc)
    {
        if (desc.vertexBufferSize == 0 && desc.indexBufferSize == 0)
            throw std::runtime_error("MeshDesc invalid: both vertex and index buffer sizes are zero.");

        uint32_t   index = allocateMeshSlot();
        MeshEntry& me    = meshes_[index];

        if (me.alive)
            me = MeshEntry{}; // Mesh 自身不拥有 Vulkan 资源，仅重置句柄与 desc

        // 顶点缓冲
        if (desc.vertexBufferSize > 0)
        {
            BufferDesc vbDesc{};
            vbDesc.size        = desc.vertexBufferSize;
            vbDesc.usage       = desc.vertexUsage;
            vbDesc.memoryFlags = desc.vertexMemoryFlags;
            vbDesc.debugName   = desc.vertexDebugName;
            me.vertexBuffer    = createBuffer(vbDesc);
        }

        // 索引缓冲（可选）
        if (desc.indexBufferSize > 0)
        {
            BufferDesc ibDesc{};
            ibDesc.size        = desc.indexBufferSize;
            ibDesc.usage       = desc.indexUsage;
            ibDesc.memoryFlags = desc.indexMemoryFlags;
            ibDesc.debugName   = desc.indexDebugName;
            me.indexBuffer     = createBuffer(ibDesc);
        }

        me.desc  = desc;
        me.alive = true;
        ++me.generation;

        return MeshHandle{index, me.generation};
    }

    void ResourceManager::destroyMesh(MeshHandle handle) noexcept
    {
        if (!handle || handle.index >= meshes_.size())
            return;

        MeshEntry& me = meshes_[handle.index];
        if (!me.alive || me.generation != handle.generation)
            return;

        // Mesh 不直接持有 Vulkan 资源，只是转发销毁其 Buffer
        if (me.vertexBuffer)
            destroyBuffer(me.vertexBuffer);
        if (me.indexBuffer)
            destroyBuffer(me.indexBuffer);

        me = MeshEntry{};
        freeMeshIndices_.push_back(handle.index);
    }

    BufferHandle ResourceManager::getMeshVertexBuffer(MeshHandle handle) const
    {
        const MeshEntry& me = getValidMeshEntry(handle);
        return me.vertexBuffer;
    }

    BufferHandle ResourceManager::getMeshIndexBuffer(MeshHandle handle) const
    {
        const MeshEntry& me = getValidMeshEntry(handle);
        return me.indexBuffer; // 可能是无效句柄（没有索引缓冲）
    }

    const MeshDesc& ResourceManager::getMeshDesc(MeshHandle handle) const
    {
        return getValidMeshEntry(handle).desc;
    }
}
