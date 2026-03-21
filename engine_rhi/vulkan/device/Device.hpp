#pragma once

// Compatibility layer: maps old vulkan::DeviceManager to new engine::rhi::Device
// This file provides backwards compatibility for engine_render module

#include "engine/rhi/Device.hpp"

namespace engine::vulkan
{
    // DeviceManager is now an alias to engine::rhi::Device
    using DeviceManager = engine::rhi::Device;
    using DeviceHandle  = engine::rhi::DeviceHandle;
} // namespace engine::vulkan