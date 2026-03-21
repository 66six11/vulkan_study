#pragma once

// Compatibility layer: Image resources (maps to Texture)

#include "engine/rhi/Texture.hpp"

namespace engine::vulkan
{
    // Image is now mapped to engine::rhi::Texture
    using Image           = rhi::Texture;
    using ImageHandle     = rhi::TextureHandle;
    using ImageView       = rhi::TextureView;
    using ImageViewHandle = rhi::TextureViewHandle;
} // namespace engine::vulkan