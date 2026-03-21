#pragma once

#include <memory>
#include <vector>
#include "Core.hpp"

namespace engine::rhi
{
    // Forward declarations
    class Texture;
    class Device;
    using TextureHandle = std::shared_ptr<Texture>;

    // Texture description
    struct TextureDesc
    {
        TextureType    type              = TextureType::Texture2D;
        Format         format            = Format::Undefined;
        Extent3D       extent            = {};
        uint32_t       mipLevels         = 1;
        uint32_t       arrayLayers       = 1;
        SampleCount    samples           = SampleCount::Samples1;
        TextureUsage   usage             = TextureUsage::None;
        MemoryProperty memoryProperties  = MemoryProperty::DeviceLocal;
        bool           createDefaultView = true;
    };

    // Texture view description
    struct TextureViewDesc
    {
        Format      format         = Format::Undefined;      // Undefined = use texture format
        TextureType viewType       = TextureType::Texture2D; // Can differ from texture type
        uint32_t    baseMipLevel   = 0;
        uint32_t    levelCount     = 1;
        uint32_t    baseArrayLayer = 0;
        uint32_t    layerCount     = 1;
        // Component swizzle can be added here if needed
    };

    // Texture view class
    class TextureView
    {
        public:
            TextureView() = default;
            ~TextureView();

            // Non-copyable
            TextureView(const TextureView&)            = delete;
            TextureView& operator=(const TextureView&) = delete;

            // Movable
            TextureView(TextureView&& other) noexcept;
            TextureView& operator=(TextureView&& other) noexcept;

            [[nodiscard]] bool                   isValid() const noexcept { return view_ != nullptr; }
            [[nodiscard]] VkImageView            nativeHandle() const noexcept { return view_; }
            [[nodiscard]] const TextureViewDesc& desc() const noexcept { return desc_; }

            // Parent texture (keeps it alive)
            [[nodiscard]] TextureHandle parentTexture() const { return parent_; }

            // Internal construction
            struct InternalData
            {
                VkImageView view   = nullptr;
                VkDevice    device = nullptr;
            };

            explicit TextureView(const InternalData& data, TextureHandle parent, const TextureViewDesc& desc);
            void     release();

        private:
            VkImageView     view_   = nullptr;
            VkDevice        device_ = nullptr;
            TextureHandle   parent_;
            TextureViewDesc desc_;
    };

    using TextureViewHandle = std::shared_ptr<TextureView>;

    // Texture copy region
    struct TextureCopyRegion
    {
        TextureSubresourceRange srcSubresource;
        Offset3D                srcOffset;
        TextureSubresourceRange dstSubresource;
        Offset3D                dstOffset;
        Extent3D                extent;
    };

    // Texture class
    class Texture
    {
        public:
            Texture() = default;
            ~Texture();

            // Non-copyable
            Texture(const Texture&)            = delete;
            Texture& operator=(const Texture&) = delete;

            // Movable
            Texture(Texture&& other) noexcept;
            Texture& operator=(Texture&& other) noexcept;

            // Properties
            [[nodiscard]] const TextureDesc& desc() const noexcept { return desc_; }
            [[nodiscard]] TextureType        type() const noexcept { return desc_.type; }
            [[nodiscard]] Format             format() const noexcept { return desc_.format; }
            [[nodiscard]] const Extent3D&    extent() const noexcept { return desc_.extent; }
            [[nodiscard]] uint32_t           width() const noexcept { return desc_.extent.width; }
            [[nodiscard]] uint32_t           height() const noexcept { return desc_.extent.height; }
            [[nodiscard]] uint32_t           depth() const noexcept { return desc_.extent.depth; }
            [[nodiscard]] uint32_t           mipLevels() const noexcept { return desc_.mipLevels; }
            [[nodiscard]] uint32_t           arrayLayers() const noexcept { return desc_.arrayLayers; }
            [[nodiscard]] SampleCount        samples() const noexcept { return desc_.samples; }
            [[nodiscard]] TextureUsage       usage() const noexcept { return desc_.usage; }
            [[nodiscard]] bool               isValid() const noexcept { return handle_ != nullptr; }

            // Native handle
            [[nodiscard]] VkImage       nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] VmaAllocation allocation() const noexcept { return allocation_; }

            // Default view (full texture)
            [[nodiscard]] TextureViewHandle defaultView() const { return defaultView_; }

            // Create specific view
            [[nodiscard]] ResultValue<TextureViewHandle> createView(const TextureViewDesc& viewDesc);

            // Internal construction
            struct InternalData
            {
                VkImage           image      = nullptr;
                VmaAllocation     allocation = nullptr;
                VmaAllocator      allocator  = nullptr;
                VkDevice          device     = nullptr;
                TextureViewHandle defaultView;
            };

            explicit Texture(const InternalData& data, const TextureDesc& desc);
            void     release();

        private:
            VkImage           handle_     = nullptr;
            VmaAllocation     allocation_ = nullptr;
            VmaAllocator      allocator_  = nullptr;
            VkDevice          device_     = nullptr;
            TextureDesc       desc_;
            TextureViewHandle defaultView_;
    };

    // Filter enum
    enum class Filter
    {
        Nearest,
        Linear,
    };

    // Sampler mipmap mode
    enum class SamplerMipmapMode
    {
        Nearest,
        Linear,
    };

    // Sampler address mode
    enum class SamplerAddressMode
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge,
    };

    // Sampler description
    struct SamplerDesc
    {
        Filter             magFilter        = Filter::Linear;
        Filter             minFilter        = Filter::Linear;
        SamplerMipmapMode  mipmapMode       = SamplerMipmapMode::Linear;
        SamplerAddressMode addressModeU     = SamplerAddressMode::Repeat;
        SamplerAddressMode addressModeV     = SamplerAddressMode::Repeat;
        SamplerAddressMode addressModeW     = SamplerAddressMode::Repeat;
        float              mipLodBias       = 0.0f;
        bool               anisotropyEnable = false;
        float              maxAnisotropy    = 1.0f;
        bool               compareEnable    = false;
        CompareOp          compareOp        = CompareOp::Always;
        float              minLod           = 0.0f;
        float              maxLod           = 1000.0f;
        // Border color can be added if needed
    };

    // Sampler class
    class Sampler
    {
        public:
            Sampler() = default;
            ~Sampler();

            Sampler(const Sampler&)            = delete;
            Sampler& operator=(const Sampler&) = delete;

            Sampler(Sampler&& other) noexcept;
            Sampler& operator=(Sampler&& other) noexcept;

            [[nodiscard]] bool               isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkSampler          nativeHandle() const noexcept { return handle_; }
            [[nodiscard]] const SamplerDesc& desc() const noexcept { return desc_; }

            struct InternalData
            {
                VkSampler sampler = nullptr;
                VkDevice  device  = nullptr;
            };

            explicit Sampler(const InternalData& data, const SamplerDesc& desc);
            void     release();

        private:
            VkSampler   handle_ = nullptr;
            VkDevice    device_ = nullptr;
            SamplerDesc desc_;
    };

    using SamplerHandle = std::shared_ptr<Sampler>;
} // namespace engine::rhi
