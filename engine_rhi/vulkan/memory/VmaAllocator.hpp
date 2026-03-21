#pragma once

// Compatibility layer: VMA allocator wrapper
// Note: VMA is now managed internally by engine::rhi::Device

#include "engine/rhi/Device.hpp"

namespace engine::vulkan::memory
{
    // VmaAllocator wraps the allocator managed by Device
    class VmaAllocator
    {
        public:
            struct CreateInfo
            {
                bool enableBudget         = false;
                bool enableMemoryPriority = false;
            };

            VmaAllocator() = default;

            VmaAllocator(std::shared_ptr<rhi::Device> device, const CreateInfo& info = {})
                : device_(std::move(device))
            {
                // VMA is now managed by Device internally
            }

            ~VmaAllocator() = default;

            // Get the underlying VMA allocator handle
            [[nodiscard]] void* handle() const
            {
                return device_ ? device_->nativeVmaAllocator() : nullptr;
            }

            [[nodiscard]] std::shared_ptr<rhi::Device> device() const { return device_; }

        private:
            std::shared_ptr<rhi::Device> device_;
    };
} // namespace engine::vulkan::memory