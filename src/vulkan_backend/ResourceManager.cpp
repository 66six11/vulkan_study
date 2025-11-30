//
// Created by C66 on 2025/11/22.
//

#include "vulkan_backend/ResourceManager.h"
#include <cstring>
#include <stdexcept>
#include "vulkan_backend/VertexInputDescription.h"

namespace
{
    uint32_t findMemoryType(
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
        throw std::runtime_error("Failed to find suitable memory type");
    }
} // namespace

ResourceManager::ResourceManager(VulkanDevice& device) : device_(device)
{
}

ResourceManager::~ResourceManager()
{
    // 逆序销毁所有资源，确保在 VkDevice 销毁前完成
    for (auto& sampler : samplers_)
    {
        if (sampler.alive)
        {
            vkDestroySampler(device_.device(), sampler.sampler, nullptr);
        }
    }

    for (auto& image : images_)
    {
        if (image.alive)
        {
            if (image.defaultView)
            {
                vkDestroyImageView(device_.device(), image.defaultView, nullptr);
            }
            if (image.image)
            {
                vkDestroyImage(device_.device(), image.image, nullptr);
            }
            if (image.memory)
            {
                vkFreeMemory(device_.device(), image.memory, nullptr);
            }
        }
    }

    for (auto& buffer : buffers_)
    {
        if (buffer.alive)
        {
            if (buffer.buffer)
            {
                vkDestroyBuffer(device_.device(), buffer.buffer, nullptr);
            }
            if (buffer.memory)
            {
                vkFreeMemory(device_.device(), buffer.memory, nullptr);
            }
        }
    }
}

ResourceManager::BufferHandle ResourceManager::createBuffer(const BufferDesc& desc)
{
    std::unique_lock lock(bufferMutex_);

    uint32_t index;
    if (!freeBufferIndices_.empty())
    {
        index = freeBufferIndices_.back();
        freeBufferIndices_.pop_back();
    }
    else
    {
        index = static_cast<uint32_t>(buffers_.size());
        buffers_.emplace_back();
    }

    BufferEntry& entry = buffers_[index];

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = desc.size;
    bufferInfo.usage       = desc.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_.device(), &bufferInfo, nullptr, &entry.buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(device_.device(), entry.buffer, &memReq);

    const auto& memProps = device_.memoryProperties();
    uint32_t    typeIdx  = findMemoryType(memReq.memoryTypeBits, desc.memoryFlags, memProps);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = typeIdx;

    if (vkAllocateMemory(device_.device(), &allocInfo, nullptr, &entry.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(device_.device(), entry.buffer, entry.memory, 0);

    entry.desc  = desc;
    entry.alive = true;
    entry.generation++;

    return BufferHandle{index, entry.generation};
}

void ResourceManager::destroyBuffer(BufferHandle handle)
{
    if (!handle)
    {
        return;
    }

    std::unique_lock lock(bufferMutex_);

    if (handle.index >= buffers_.size())
    {
        return;
    }

    BufferEntry& entry = buffers_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return;
    }

    if (entry.buffer)
    {
        vkDestroyBuffer(device_.device(), entry.buffer, nullptr);
    }
    if (entry.memory)
    {
        vkFreeMemory(device_.device(), entry.memory, nullptr);
    }

    entry = {};
    freeBufferIndices_.push_back(handle.index);
}

ResourceManager::ImageHandle ResourceManager::createImage(const ImageDesc& desc)
{
    std::unique_lock lock(imageMutex_);

    uint32_t index;
    if (!freeImageIndices_.empty())
    {
        index = freeImageIndices_.back();
        freeImageIndices_.pop_back();
    }
    else
    {
        index = static_cast<uint32_t>(images_.size());
        images_.emplace_back();
    }

    ImageEntry& entry = images_[index];

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent        = desc.extent;
    imageInfo.mipLevels     = desc.mipLevels;
    imageInfo.arrayLayers   = desc.arrayLayers;
    imageInfo.format        = desc.format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = desc.usage;
    imageInfo.samples       = desc.samples;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device_.device(), &imageInfo, nullptr, &entry.image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(device_.device(), entry.image, &memReq);

    const auto& memProps = device_.memoryProperties();
    uint32_t    typeIdx  = findMemoryType(memReq.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          memProps);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = typeIdx;

    if (vkAllocateMemory(device_.device(), &allocInfo, nullptr, &entry.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(device_.device(), entry.image, entry.memory, 0);

    // 创建默认视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = entry.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = desc.format;
    viewInfo.subresourceRange.aspectMask     = desc.aspect;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = desc.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = desc.arrayLayers;

    if (vkCreateImageView(device_.device(), &viewInfo, nullptr, &entry.defaultView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image view");
    }

    entry.desc  = desc;
    entry.alive = true;
    entry.generation++;

    return ImageHandle{index, entry.generation};
}

void ResourceManager::destroyImage(ImageHandle handle)
{
    std::unique_lock lock(imageMutex_);

    if (handle.index >= images_.size())
    {
        return;
    }

    ImageEntry& entry = images_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return;
    }

    if (entry.defaultView)
    {
        vkDestroyImageView(device_.device(), entry.defaultView, nullptr);
    }
    if (entry.image)
    {
        vkDestroyImage(device_.device(), entry.image, nullptr);
    }
    if (entry.memory)
    {
        vkFreeMemory(device_.device(), entry.memory, nullptr);
    }

    entry = {};
    freeImageIndices_.push_back(handle.index);
}

ResourceManager::SamplerHandle ResourceManager::createSampler(
    const VkSamplerCreateInfo& info,
    std::string_view           debugName)
{
    std::unique_lock lock(samplerMutex_);

    uint32_t index;
    if (!freeSamplerIndices_.empty())
    {
        index = freeSamplerIndices_.back();
        freeSamplerIndices_.pop_back();
    }
    else
    {
        index = static_cast<uint32_t>(samplers_.size());
        samplers_.emplace_back();
    }

    SamplerEntry& entry = samplers_[index];

    if (vkCreateSampler(device_.device(), &info, nullptr, &entry.sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create sampler");
    }

    entry.name  = std::string(debugName);
    entry.alive = true;
    entry.generation++;

    return SamplerHandle{index, entry.generation};
}

void ResourceManager::destroySampler(SamplerHandle handle)
{
    std::unique_lock lock(samplerMutex_);

    if (handle.index >= samplers_.size())
    {
        return;
    }

    SamplerEntry& entry = samplers_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return;
    }

    if (entry.sampler)
    {
        vkDestroySampler(device_.device(), entry.sampler, nullptr);
    }

    entry = {};
    freeSamplerIndices_.push_back(handle.index);
}

ResourceManager::MeshHandle ResourceManager::createMesh(
    const void*      vertexData,
    size_t           vertexCount,
    const void*      indexData,
    size_t           indexCount,
    std::string_view debugName)
{
    std::unique_lock lock(meshMutex_);

    uint32_t index;

    if (!freeMeshIndices_.empty())
    {
        index = freeMeshIndices_.back();
        freeMeshIndices_.pop_back();
    }
    else
    {
        index = static_cast<uint32_t>(meshes_.size());
        meshes_.emplace_back();
    }

    MeshEntry& entry = meshes_[index];

    MeshDesc desc = {};

    //验证顶点数据
    if (vertexData == nullptr || vertexCount == 0)
    {
        throw std::runtime_error("Invalid vertex data for mesh creation");
    }
    //填充 MeshDesc
    desc.vertexCount = vertexCount;
    desc.indexCount  = indexCount;
    desc.debugName   = std::string(debugName) + "_mesh_desc";

    //计算顶点大小
    size_t vertexDataSize = vertexCount * sizeof(Vertex);

    //创建顶点缓冲区
    BufferDesc vertexBufferDesc  = {};
    vertexBufferDesc.size        = vertexDataSize;
    vertexBufferDesc.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferDesc.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vertexBufferDesc.debugName   = std::string(debugName) + "_vertex_buffer";
    entry.vertexBuffer           = createBuffer(vertexBufferDesc);

    //创建索引缓冲区（如果有索引数据）
    if (indexData != nullptr && indexCount > 0)
    {
        BufferDesc indexBufferDesc  = {};
        size_t     indexDataSize    = indexCount * sizeof(uint32_t);
        indexBufferDesc.size        = indexDataSize;
        indexBufferDesc.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBufferDesc.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        indexBufferDesc.debugName   = std::string(debugName) + "_index_buffer";
        entry.indexBuffer           = createBuffer(indexBufferDesc);
    }


    entry.alive = true;
    entry.generation++;

    return MeshHandle{index, entry.generation};
}

void ResourceManager::destroyMesh(MeshHandle handle)
{
    std::unique_lock lock(meshMutex_);

    if (handle.index >= meshes_.size())
    {
        return;
    }

    MeshEntry& entry = meshes_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return;
    }

    destroyBuffer(entry.vertexBuffer);
    if (entry.indexBuffer)
    {
        destroyBuffer(entry.indexBuffer);
    }

    entry = {};
    freeMeshIndices_.push_back(handle.index);
}

VkBuffer ResourceManager::getBuffer(BufferHandle handle) const
{
    std::shared_lock lock(bufferMutex_);

    if (!handle || handle.index >= buffers_.size())
    {
        return VK_NULL_HANDLE;
    }

    const BufferEntry& entry = buffers_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return VK_NULL_HANDLE;
    }

    return entry.buffer;
}

VkImage ResourceManager::getImage(ImageHandle handle) const
{
    std::shared_lock lock(imageMutex_);

    if (handle.index >= images_.size())
    {
        return VK_NULL_HANDLE;
    }

    const ImageEntry& entry = images_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return VK_NULL_HANDLE;
    }

    return entry.image;
}

VkImageView ResourceManager::getImageView(ImageHandle handle) const
{
    std::shared_lock lock(imageMutex_);

    if (handle.index >= images_.size())
    {
        return VK_NULL_HANDLE;
    }

    const ImageEntry& entry = images_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return VK_NULL_HANDLE;
    }

    return entry.defaultView;
}

VkSampler ResourceManager::getSampler(SamplerHandle handle) const
{
    std::shared_lock lock(samplerMutex_);

    if (handle.index >= samplers_.size())
    {
        return VK_NULL_HANDLE;
    }

    const SamplerEntry& entry = samplers_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return VK_NULL_HANDLE;
    }

    return entry.sampler;
}

ResourceManager::BufferHandle ResourceManager::getMeshVertexBuffer(MeshHandle handle) const
{
    std::shared_lock lock(meshMutex_);

    if (handle.index >= meshes_.size())
    {
        throw std::runtime_error("Invalid mesh handle");
    }

    const MeshEntry& entry = meshes_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        throw std::runtime_error("Mesh handle is not alive or generation mismatch");
    }

    return entry.vertexBuffer;
}
ResourceManager::BufferHandle ResourceManager::getMeshIndexBuffer(MeshHandle handle) const
{
    std::shared_lock lock(meshMutex_);

    if (handle.index >= meshes_.size())
    {
        throw std::runtime_error("Invalid mesh handle");
    }

    const MeshEntry& entry = meshes_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        throw std::runtime_error("Mesh handle is not alive or generation mismatch");
    }

    return entry.indexBuffer;
}

const ResourceManager::BufferDesc& ResourceManager::getBufferDesc(BufferHandle handle) const
{
    std::shared_lock lock(bufferMutex_);
    return buffers_[handle.index].desc;
}

const ResourceManager::ImageDesc& ResourceManager::getImageDesc(ImageHandle handle) const
{
    std::shared_lock lock(imageMutex_);
    return images_[handle.index].desc;
}

const ResourceManager::MeshDesc& ResourceManager::getMeshDesc(MeshHandle handle) const
{
    std::shared_lock lock(meshMutex_);
    return meshes_[handle.index].desc;
}

void ResourceManager::uploadBuffer(BufferHandle handle, const void* data, VkDeviceSize size, VkDeviceSize offset)
{
    std::shared_lock lock(bufferMutex_);

    if (!handle || handle.index >= buffers_.size())
    {
        return;
    }

    BufferEntry& entry = buffers_[handle.index];
    if (!entry.alive || entry.generation != handle.generation)
    {
        return;
    }

    void* mapped = nullptr;
    if (vkMapMemory(device_.device(), entry.memory, offset, size, 0, &mapped) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map buffer memory");
    }

    std::memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(device_.device(), entry.memory);
}

void ResourceManager::garbageCollect()
{
    // 当前实现为空，占位以支持未来的延迟销毁/分帧回收策略
}
