#include "engine/rhi/vulkan/memory/VmaBuffer.hpp"
#include "engine/core/utils/Logger.hpp"
#include <cstring>
#include <sstream>

namespace vulkan_engine::vulkan::memory
{
    VmaBuffer::VmaBuffer(
        std::shared_ptr<VmaAllocator>  allocator,
        VkDeviceSize                   size,
        VkBufferUsageFlags             usage,
        const VmaAllocationCreateInfo& allocInfo)
        : allocator_(std::move(allocator))
        , size_(size)
        , usage_(usage)
    {
        if (!allocator_)
        {
            throw std::runtime_error("VmaBuffer: allocator is null");
        }

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = size;
        bufferInfo.usage              = usage;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocation     allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo;

        VkResult result = vmaCreateBuffer(allocator_->handle(), &bufferInfo, &allocInfo, &buffer_, &allocation, &allocationInfo);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create VMA buffer", __FILE__, __LINE__);
        }

        allocation_ = Allocation(allocator_, allocation);

        // 瀛樺偍鍒嗛厤淇℃伅
        allocationInfo_.size               = allocationInfo.size;
        allocationInfo_.memoryTypeIndex    = allocationInfo.memoryType;
        allocationInfo_.mappedData         = allocationInfo.pMappedData;
        allocationInfo_.isPersistentMapped = (allocInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

        // 濡傛灉宸茬粡鎸佷箙鏄犲皠锛屼繚瀛樻槧灏勬寚閽?
        if (allocationInfo_.isPersistentMapped && allocationInfo.pMappedData != nullptr)
        {
            mappedData_ = allocationInfo.pMappedData;
        }

        std::ostringstream oss;
        oss << "VmaBuffer created: size=" << size / (1024.0 * 1024.0)
                << " MB, usage=0x" << std::hex << usage
                << ", memoryType=" << allocationInfo_.memoryTypeIndex;
        LOG_DEBUG(oss.str());
    }

    VmaBuffer::~VmaBuffer()
    {
        cleanup();
    }

    VmaBuffer::VmaBuffer(VmaBuffer&& other) noexcept
        : allocator_(std::move(other.allocator_))
        , buffer_(other.buffer_)
        , allocation_(std::move(other.allocation_))
        , size_(other.size_)
        , usage_(other.usage_)
        , allocationInfo_(other.allocationInfo_)
        , mappedData_(other.mappedData_)
    {
        other.buffer_     = VK_NULL_HANDLE;
        other.size_       = 0;
        other.mappedData_ = nullptr;
    }

    VmaBuffer& VmaBuffer::operator=(VmaBuffer&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            allocator_        = std::move(other.allocator_);
            buffer_           = other.buffer_;
            allocation_       = std::move(other.allocation_);
            size_             = other.size_;
            usage_            = other.usage_;
            allocationInfo_   = other.allocationInfo_;
            mappedData_       = other.mappedData_;
            other.buffer_     = VK_NULL_HANDLE;
            other.size_       = 0;
            other.mappedData_ = nullptr;
        }
        return *this;
    }

    void VmaBuffer::cleanup() noexcept
    {
        if (buffer_ != VK_NULL_HANDLE)
        {
            // Allocation 鏋愭瀯鏃朵細鑷姩閲婃斁鍐呭瓨
            // 鍙渶瑕侀攢姣?buffer 瀵硅薄
            if (auto allocator = allocator_)
            {
                vmaDestroyBuffer(allocator->handle(), buffer_, VK_NULL_HANDLE);
            }
            buffer_ = VK_NULL_HANDLE;
        }
    }

    void* VmaBuffer::map()
    {
        if (mappedData_ != nullptr)
        {
            return mappedData_;
        }
        return allocation_.map();
    }

    void VmaBuffer::unmap()
    {
        if (!allocationInfo_.isPersistentMapped)
        {
            allocation_.unmap();
            mappedData_ = nullptr;
        }
    }

    void VmaBuffer::write(const void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        if (offset + size > size_)
        {
            throw std::runtime_error("VmaBuffer::write: out of bounds");
        }

        void* mapped = map();
        if (!mapped)
        {
            throw std::runtime_error("VmaBuffer::write: failed to map buffer");
        }

        std::memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));

        // 濡傛灉涓嶆槸 coherent 鍐呭瓨锛岄渶瑕佸埛鏂?
        if (!allocationInfo_.isPersistentMapped || (allocationInfo_.memoryTypeIndex & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            flush(offset, size);
        }
    }

    void VmaBuffer::write(const std::span<const std::byte>& data, VkDeviceSize offset)
    {
        write(data.data(), data.size(), offset);
    }

    void VmaBuffer::read(void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        if (offset + size > size_)
        {
            throw std::runtime_error("VmaBuffer::read: out of bounds");
        }

        void* mapped = map();
        if (!mapped)
        {
            throw std::runtime_error("VmaBuffer::read: failed to map buffer");
        }

        // 濡傛灉涓嶆槸 coherent 鍐呭瓨锛岄渶瑕佷娇鏃犳晥
        if (!allocationInfo_.isPersistentMapped || (allocationInfo_.memoryTypeIndex & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            invalidate(offset, size);
        }

        std::memcpy(data, static_cast<char*>(mapped) + offset, static_cast<size_t>(size));
    }

    void VmaBuffer::copyFrom(const VmaBuffer& source, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
    {
        if (dstOffset + size > size_ || srcOffset + size > source.size_)
        {
            throw std::runtime_error("VmaBuffer::copyFrom: out of bounds");
        }

        // 濡傛灉涓や釜 buffer 閮芥槸 host-visible锛屽彲浠ョ洿鎺ユ嫹璐?
        if (isMapped() && source.isMapped())
        {
            void* srcData = const_cast<VmaBuffer&>(source).map();
            void* dstData = map();
            std::memcpy(static_cast<char*>(dstData) + dstOffset,
                        static_cast<const char*>(srcData) + srcOffset,
                        static_cast<size_t>(size));
        }
        else
        {
            throw std::runtime_error("VmaBuffer::copyFrom: buffers must be host-visible for CPU copy");
        }
    }

    void VmaBuffer::flush(VkDeviceSize offset, VkDeviceSize size)
    {
        allocation_.flush(offset, size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : size);
    }

    void VmaBuffer::invalidate(VkDeviceSize offset, VkDeviceSize size)
    {
        allocation_.invalidate(offset, size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : size);
    }

    VkDeviceAddress VmaBuffer::deviceAddress() const
    {
        if (buffer_ == VK_NULL_HANDLE)
        {
            return 0;
        }

        VkBufferDeviceAddressInfo info = {};
        info.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer                    = buffer_;

        if (auto allocator = allocator_)
        {
            return vkGetBufferDeviceAddress(allocator->device()->device().handle(), &info);
        }
        return 0;
    }

    VkDeviceSize VmaBuffer::getAlignmentRequirements(
        std::shared_ptr<VmaAllocator> allocator,
        VkDeviceSize                  size,
        VkBufferUsageFlags            usage)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = size;
        bufferInfo.usage              = usage;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer = VK_NULL_HANDLE;
        VkResult result = vkCreateBuffer(allocator->device()->device().handle(), &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS)
        {
            return 0;
        }

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(allocator->device()->device().handle(), buffer, &memReqs);
        vkDestroyBuffer(allocator->device()->device().handle(), buffer, nullptr);

        return memReqs.alignment;
    }

    // VmaBufferBuilder 瀹炵幇
    VmaBufferBuilder::VmaBufferBuilder(std::shared_ptr<VmaAllocator> allocator)
        : allocator_(std::move(allocator))
    {
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO;
    }

    VmaBufferBuilder& VmaBufferBuilder::size(VkDeviceSize size)
    {
        size_ = size;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::usage(VkBufferUsageFlags usage)
    {
        usage_ = usage;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::hostVisible(bool persistentMap)
    {
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo_.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo_.preferredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (persistentMap)
        {
            allocInfo_.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            // VMA 瑕佹眰锛氫娇鐢?MAPPED_BIT 鏃跺繀椤绘寚瀹?HOST_ACCESS 鏍囧織
            allocInfo_.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::hostCached()
    {
        allocInfo_.preferredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::deviceLocal()
    {
        allocInfo_.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo_.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::sequentialWrite()
    {
        allocInfo_.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::createMapped()
    {
        allocInfo_.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::pool(VmaPool pool)
    {
        allocInfo_.pool = pool;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::priority(float priority)
    {
        allocInfo_.priority = priority;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::allocationFlags(VmaAllocationCreateFlags flags)
    {
        allocInfo_.flags = flags;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::requiredFlags(VkMemoryPropertyFlags flags)
    {
        allocInfo_.requiredFlags = flags;
        return *this;
    }

    VmaBufferBuilder& VmaBufferBuilder::preferredFlags(VkMemoryPropertyFlags flags)
    {
        allocInfo_.preferredFlags = flags;
        return *this;
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::build()
    {
        return std::make_unique < VmaBuffer > (allocator_, size_, usage_, allocInfo_);
    }

    VmaBufferPtr VmaBufferBuilder::buildShared()
    {
        return std::make_shared < VmaBuffer > (allocator_, size_, usage_, allocInfo_);
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createStagingBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size)
    {
        return VmaBufferBuilder(allocator)
              .size(size)
              .usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
              .hostVisible(true)
              .sequentialWrite()
              .build();
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createVertexBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size)
    {
        return VmaBufferBuilder(allocator)
              .size(size)
              .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
              .deviceLocal()
              .build();
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createIndexBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size)
    {
        return VmaBufferBuilder(allocator)
              .size(size)
              .usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
              .deviceLocal()
              .build();
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createUniformBuffer(
        std::shared_ptr<VmaAllocator> allocator,
        VkDeviceSize                  size,
        bool                          persistentMap)
    {
        return VmaBufferBuilder(allocator)
              .size(size)
              .usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
              .hostVisible(persistentMap)
              .build();
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createStorageBuffer(
        std::shared_ptr<VmaAllocator> allocator,
        VkDeviceSize                  size,
        bool                          hostVisible)
    {
        VmaBufferBuilder builder(allocator);
        builder.size(size)
               .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        if (hostVisible)
        {
            builder.hostVisible(true);
        }
        else
        {
            builder.deviceLocal();
        }

        return builder.build();
    }

    std::unique_ptr<VmaBuffer> VmaBufferBuilder::createIndirectBuffer(std::shared_ptr<VmaAllocator> allocator, VkDeviceSize size)
    {
        return VmaBufferBuilder(allocator)
              .size(size)
              .usage(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
              .deviceLocal()
              .build();
    }
} // namespace vulkan_engine::vulkan::memory