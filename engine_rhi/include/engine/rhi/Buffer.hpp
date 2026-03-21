#pragma once

#include <memory>
#include "Core.hpp"

namespace engine::rhi
{
    // Forward declaration
    class Device;

    // Buffer description
    struct BufferDesc
    {
        uint32_t       size             = 0;
        uint32_t       stride           = 0; // For structured buffers
        BufferUsage    usage            = BufferUsage::None;
        MemoryProperty memoryProperties = MemoryProperty::DeviceLocal;
        bool           persistentMap    = false; // Keep mapped after creation
    };

    // Buffer class - lightweight RAII wrapper
    class Buffer
    {
        public:
            Buffer() = default;
            ~Buffer();

            // Non-copyable
            Buffer(const Buffer&)            = delete;
            Buffer& operator=(const Buffer&) = delete;

            // Movable
            Buffer(Buffer&& other) noexcept;
            Buffer& operator=(Buffer&& other) noexcept;

            // Properties
            [[nodiscard]] uint32_t       size() const noexcept { return size_; }
            [[nodiscard]] uint32_t       stride() const noexcept { return stride_; }
            [[nodiscard]] BufferUsage    usage() const noexcept { return usage_; }
            [[nodiscard]] MemoryProperty memoryProperties() const noexcept { return memoryProperties_; }
            [[nodiscard]] bool           isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] bool           isMapped() const noexcept { return mappedData_ != nullptr; }

            // Mapping (only for HostVisible memory)
            [[nodiscard]] ResultValue<void*> map();
            void                             unmap();
            [[nodiscard]] void*              mappedData() const noexcept { return mappedData_; }

            // Direct memory write (if mapped)
            Result writeData(const void* data, uint32_t dataSize, uint32_t offset = 0);

            // Native handle access
            [[nodiscard]] VkBuffer      nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] VmaAllocation allocation() const noexcept { return allocation_; }

            // Internal construction data
            struct InternalData
            {
                VkBuffer      buffer     = nullptr;
                VmaAllocation allocation = nullptr;
                VmaAllocator  allocator  = nullptr; // VMA allocator reference
                void*         mappedPtr  = nullptr; // Pre-mapped pointer if persistent
            };

            explicit Buffer(const InternalData& data, const BufferDesc& desc);

            // Release resources
            void release();

        private:
            VkBuffer      handle_     = nullptr;
            VmaAllocation allocation_ = nullptr;
            VmaAllocator  allocator_  = nullptr;
            void*         mappedData_ = nullptr;

            uint32_t       size_   = 0;
            uint32_t       stride_ = 0;
            BufferUsage    usage_;
            MemoryProperty memoryProperties_;
            bool           persistentMap_ = false;
    };

    using BufferHandle = std::shared_ptr<Buffer>;

    // Buffer view description (for structured buffers)
    struct BufferViewDesc
    {
        Format   format = Format::Undefined;
        uint32_t offset = 0;
        uint32_t range  = 0;
    };

    // Buffer copy region
    struct BufferCopyRegion
    {
        uint32_t srcOffset = 0;
        uint32_t dstOffset = 0;
        uint32_t size      = 0;
    };

    // Buffer to texture copy region
    struct BufferTextureCopyRegion
    {
        uint32_t                bufferOffset      = 0;
        uint32_t                bufferRowLength   = 0; // 0 = tightly packed
        uint32_t                bufferImageHeight = 0; // 0 = tightly packed
        TextureSubresourceRange imageSubresource;
        Offset3D                imageOffset;
        Extent3D                imageExtent;
    };
} // namespace engine::rhi
