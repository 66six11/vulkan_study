#pragma once

// Compatibility layer: Framebuffer
// Note: With Dynamic Rendering (Vulkan 1.3+), framebuffers are largely obsolete
// This is a minimal compatibility shim

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace engine::vulkan
{
    // Framebuffer - minimal implementation for legacy code
    // Modern code should use Dynamic Rendering instead
    class Framebuffer
    {
        public:
            Framebuffer()  = default;
            ~Framebuffer() = default;

            // Non-copyable
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            [[nodiscard]] VkFramebuffer handle() const { return handle_; }
            [[nodiscard]] uint32_t      width() const { return width_; }
            [[nodiscard]] uint32_t      height() const { return height_; }

        private:
            VkFramebuffer handle_ = VK_NULL_HANDLE;
            uint32_t      width_  = 0;
            uint32_t      height_ = 0;
    };

    // FramebufferPool - manages framebuffer caching
    // With Dynamic Rendering, this is essentially a no-op
    class FramebufferPool
    {
        public:
            FramebufferPool()  = default;
            ~FramebufferPool() = default;

            // No-op for Dynamic Rendering
            void clear()
            {
            }

            void release_all()
            {
            }
    };
} // namespace engine::vulkan