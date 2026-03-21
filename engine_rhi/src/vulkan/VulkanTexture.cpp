#include "engine/rhi/Texture.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace engine::rhi
{
    // Helper to convert Format to VkFormat
    static VkFormat convertFormat(Format format)
    {
        switch (format)
        {
            case Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
            case Format::R8G8_UNORM: return VK_FORMAT_R8G8_UNORM;
            case Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case Format::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case Format::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case Format::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case Format::R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
            case Format::R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
            case Format::R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
            case Format::R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Format::R32_UINT: return VK_FORMAT_R32_UINT;
            case Format::R32G32_UINT: return VK_FORMAT_R32G32_UINT;
            case Format::R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT;
            case Format::R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
            case Format::D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
            case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
            case Format::D16_UNORM: return VK_FORMAT_D16_UNORM;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    // Helper to convert TextureType to VkImageType
    static VkImageType convertImageType(TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture1D:
            case TextureType::Texture1DArray:
                return VK_IMAGE_TYPE_1D;
            case TextureType::Texture2D:
            case TextureType::Texture2DArray:
            case TextureType::TextureCube:
            case TextureType::TextureCubeArray:
                return VK_IMAGE_TYPE_2D;
            case TextureType::Texture3D:
                return VK_IMAGE_TYPE_3D;
            default:
                return VK_IMAGE_TYPE_2D;
        }
    }

    // Helper to convert TextureType to VkImageViewType
    static VkImageViewType convertViewType(TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture1D: return VK_IMAGE_VIEW_TYPE_1D;
            case TextureType::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TextureType::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureType::Texture1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            case TextureType::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case TextureType::TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            default: return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    // Helper to convert TextureUsage to VkImageUsageFlags
    static VkImageUsageFlags convertUsage(TextureUsage usage)
    {
        VkImageUsageFlags flags = 0;
        if (hasFlag(usage, TextureUsage::TransferSrc)) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (hasFlag(usage, TextureUsage::TransferDst)) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (hasFlag(usage, TextureUsage::Sampled)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (hasFlag(usage, TextureUsage::Storage)) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (hasFlag(usage, TextureUsage::ColorAttachment)) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (hasFlag(usage, TextureUsage::DepthStencilAttachment)) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (hasFlag(usage, TextureUsage::TransientAttachment)) flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        if (hasFlag(usage, TextureUsage::InputAttachment)) flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        return flags;
    }

    // Helper to convert SampleCount to VkSampleCountFlagBits
    static VkSampleCountFlagBits convertSamples(SampleCount samples)
    {
        switch (samples)
        {
            case SampleCount::Samples1: return VK_SAMPLE_COUNT_1_BIT;
            case SampleCount::Samples2: return VK_SAMPLE_COUNT_2_BIT;
            case SampleCount::Samples4: return VK_SAMPLE_COUNT_4_BIT;
            case SampleCount::Samples8: return VK_SAMPLE_COUNT_8_BIT;
            case SampleCount::Samples16: return VK_SAMPLE_COUNT_16_BIT;
            case SampleCount::Samples32: return VK_SAMPLE_COUNT_32_BIT;
            default: return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    // Helper to get aspect mask from format
    static VkImageAspectFlags getAspectMask(Format format)
    {
        switch (format)
        {
            case Format::D32_FLOAT:
            case Format::D16_UNORM:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            case Format::D24_UNORM_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    // TextureView implementation
    TextureView::~TextureView()
    {
        release();
    }

    TextureView::TextureView(TextureView&& other) noexcept
        : view_(other.view_)
        , device_(other.device_)
        , parent_(std::move(other.parent_))
        , desc_(other.desc_)
    {
        other.view_   = nullptr;
        other.device_ = nullptr;
    }

    TextureView& TextureView::operator=(TextureView&& other) noexcept
    {
        if (this != &other)
        {
            release();
            view_         = other.view_;
            device_       = other.device_;
            parent_       = std::move(other.parent_);
            desc_         = other.desc_;
            other.view_   = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    TextureView::TextureView(const InternalData& data, TextureHandle parent, const TextureViewDesc& desc)
        : view_(data.view)
        , device_(data.device)
        , parent_(std::move(parent))
        , desc_(desc)
    {
    }

    void TextureView::release()
    {
        if (view_ && device_)
        {
            vkDestroyImageView(device_, view_, nullptr);
            view_   = nullptr;
            device_ = nullptr;
            parent_.reset();
        }
    }

    // Texture implementation
    Texture::~Texture()
    {
        release();
    }

    Texture::Texture(Texture&& other) noexcept
        : handle_(other.handle_)
        , allocation_(other.allocation_)
        , allocator_(other.allocator_)
        , device_(other.device_)
        , desc_(other.desc_)
        , defaultView_(std::move(other.defaultView_))
    {
        other.handle_     = nullptr;
        other.allocation_ = nullptr;
        other.allocator_  = nullptr;
        other.device_     = nullptr;
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_           = other.handle_;
            allocation_       = other.allocation_;
            allocator_        = other.allocator_;
            device_           = other.device_;
            desc_             = other.desc_;
            defaultView_      = std::move(other.defaultView_);
            other.handle_     = nullptr;
            other.allocation_ = nullptr;
            other.allocator_  = nullptr;
            other.device_     = nullptr;
        }
        return *this;
    }

    Texture::Texture(const InternalData& data, const TextureDesc& desc)
        : handle_(data.image)
        , allocation_(data.allocation)
        , allocator_(data.allocator)
        , device_(data.device)
        , desc_(desc)
        , defaultView_(data.defaultView)
    {
    }

    void Texture::release()
    {
        // View must be destroyed before image
        defaultView_.reset();

        if (handle_ && allocator_)
        {
            vmaDestroyImage(allocator_, handle_, allocation_);
            handle_     = nullptr;
            allocation_ = nullptr;
            allocator_  = nullptr;
            device_     = nullptr;
        }
    }

    ResultValue<TextureViewHandle> Texture::createView(const TextureViewDesc& viewDesc)
    {
        if (!handle_ || !device_)
        {
            return std::unexpected(Result::Error_InvalidParameter);
        }

        // Determine view type
        VkImageViewType viewType = convertViewType(viewDesc.viewType);

        // Determine format
        VkFormat format = (viewDesc.format == Format::Undefined) ? convertFormat(desc_.format) : convertFormat(viewDesc.format);

        VkImageViewCreateInfo viewInfo           = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = handle_;
        viewInfo.viewType                        = viewType;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = getAspectMask(desc_.format);
        viewInfo.subresourceRange.baseMipLevel   = viewDesc.baseMipLevel;
        viewInfo.subresourceRange.levelCount     = viewDesc.levelCount;
        viewInfo.subresourceRange.baseArrayLayer = viewDesc.baseArrayLayer;
        viewInfo.subresourceRange.layerCount     = viewDesc.layerCount;

        VkImageView view;
        VkResult    result = vkCreateImageView(device_, &viewInfo, nullptr, &view);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        TextureView::InternalData internalData{
            .view = view,
            .device = device_
        };

        // Note: parent_ is nullptr here - caller should manage lifetime
        // For proper shared_from_this, Texture would need to inherit from enable_shared_from_this
        return std::make_shared<TextureView>(internalData, nullptr, viewDesc);
    }

    // Sampler helper conversions
    static VkFilter convertFilter(Filter filter)
    {
        return (filter == Filter::Linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    }

    static VkSamplerMipmapMode convertMipmapMode(SamplerMipmapMode mode)
    {
        return (mode == SamplerMipmapMode::Linear) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }

    static VkSamplerAddressMode convertAddressMode(SamplerAddressMode mode)
    {
        switch (mode)
        {
            case SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerAddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case SamplerAddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerAddressMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SamplerAddressMode::MirrorClampToEdge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    static VkCompareOp convertCompareOp(CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Never: return VK_COMPARE_OP_NEVER;
            case CompareOp::Less: return VK_COMPARE_OP_LESS;
            case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
            default: return VK_COMPARE_OP_NEVER;
        }
    }

    // Sampler implementation
    Sampler::~Sampler()
    {
        release();
    }

    Sampler::Sampler(Sampler&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , desc_(other.desc_)
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    Sampler& Sampler::operator=(Sampler&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            desc_         = other.desc_;
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    Sampler::Sampler(const InternalData& data, const SamplerDesc& desc)
        : handle_(data.sampler)
        , device_(data.device)
        , desc_(desc)
    {
    }

    void Sampler::release()
    {
        if (handle_ && device_)
        {
            vkDestroySampler(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }
} // namespace engine::rhi
