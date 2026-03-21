#pragma once

// Compatibility layer: Buffer resources

#include "engine/rhi/Buffer.hpp"

namespace engine::vulkan
{
    // Buffer is now an alias to engine::rhi::Buffer
    using Buffer       = rhi::Buffer;
    using BufferHandle = rhi::BufferHandle;
} // namespace engine::vulkan