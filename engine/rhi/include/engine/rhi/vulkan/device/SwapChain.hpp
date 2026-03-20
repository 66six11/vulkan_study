#pragma once

#include "engine/rhi/vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <functional>

// Forward declaration
namespace vulkan_engine::platform
{
    class Window;
}

namespace vulkan_engine::vulkan
{
    // SwapChain configuration
    struct SwapChainConfig
    {
        VkFormat          preferred_format       = VK_FORMAT_B8G8R8A8_UNORM;
        VkColorSpaceKHR   preferred_color_space  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkPresentModeKHR  preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR; // VSync on
        uint32_t          preferred_image_count  = 3;                        // Triple buffering
        VkImageUsageFlags image_usage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    };

    // SwapChain image wrapper
    struct SwapChainImage
    {
        VkImage     image = VK_NULL_HANDLE;
        VkImageView view  = VK_NULL_HANDLE;
        uint32_t    index = 0;
    };

    // SwapChain recreation callback
    using SwapChainRecreateCallback = std::function<void(uint32_t width, uint32_t height)>;

    class SwapChain
    {
        public:
            SwapChain(
                std::shared_ptr<DeviceManager>                   device,
                std::shared_ptr<vulkan_engine::platform::Window> window,
                const SwapChainConfig&                           config = {});
            ~SwapChain();

            // Non-copyable
            SwapChain(const SwapChain&)            = delete;
            SwapChain& operator=(const SwapChain&) = delete;

            // Movable
            SwapChain(SwapChain&& other) noexcept;
            SwapChain& operator=(SwapChain&& other) noexcept;

            // Initialize/Shutdown
            bool initialize();
            void shutdown();

            // Recreate swap chain (e.g., on window resize)
            bool recreate();

            // Acquire next image for rendering
            // Returns image index and sets image_available semaphore
            bool acquire_next_image(
                VkSemaphore image_available_semaphore,
                VkFence     fence           = VK_NULL_HANDLE,
                uint32_t&   out_image_index = default_image_index_);

            // Present the rendered image
            bool present(
                VkQueue     present_queue,
                uint32_t    image_index,
                VkSemaphore render_finished_semaphore,
                VkResult*   out_present_result = nullptr);

            // Wait for all present queue operations to complete
            void wait_for_present_queue();

            // Properties
            VkFormat              format() const { return format_; }
            VkExtent2D            extent() const { return extent_; }
            uint32_t              image_count() const { return static_cast<uint32_t>(images_.size()); }
            const SwapChainImage& get_image(uint32_t index) const { return images_[index]; }
            VkSwapchainKHR        handle() const { return swap_chain_; }

            uint32_t     width() const { return extent_.width; }
            uint32_t     height() const { return extent_.height; }
            VkRenderPass default_render_pass() const { return default_render_pass_; }

            // Callbacks
            void on_recreate(SwapChainRecreateCallback callback) { recreate_callback_ = std::move(callback); }

            // Check if swap chain needs recreation
            bool needs_recreation() const { return needs_recreation_; }

            // Create default render pass for this swap chain
            bool create_default_render_pass();

            // Create render pass with depth attachment
            bool create_render_pass_with_depth(VkFormat depth_format);

            // Get present queue family index
            uint32_t present_queue_family() const { return present_queue_family_; }

        private:
            std::shared_ptr<DeviceManager>                   device_;
            std::shared_ptr<vulkan_engine::platform::Window> window_;
            SwapChainConfig                                  config_;

            // Vulkan objects
            VkSurfaceKHR   surface_             = VK_NULL_HANDLE;
            VkSwapchainKHR swap_chain_          = VK_NULL_HANDLE;
            VkRenderPass   default_render_pass_ = VK_NULL_HANDLE;

            // Swap chain properties
            VkFormat                      format_       = VK_FORMAT_UNDEFINED;
            VkColorSpaceKHR               color_space_  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            VkPresentModeKHR              present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
            VkExtent2D                    extent_{0, 0};
            VkSurfaceTransformFlagBitsKHR transform_ = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

            // Images
            std::vector<SwapChainImage> images_;

            // Queue families
            uint32_t graphics_queue_family_ = UINT32_MAX;
            uint32_t present_queue_family_  = UINT32_MAX;
            bool     queues_are_same_       = true;

            // State
            bool            needs_recreation_ = false;
            static uint32_t default_image_index_;

            // Callbacks
            SwapChainRecreateCallback recreate_callback_;

            // Helper methods
            bool create_surface();
            bool select_surface_format();
            bool select_present_mode();
            bool select_extent();
            bool create_swap_chain();
            bool create_image_views();
            void cleanup_image_views();
            void cleanup_swap_chain();

            // Support queries
            std::vector<VkSurfaceFormatKHR> get_surface_formats() const;
            std::vector<VkPresentModeKHR>   get_present_modes() const;
            VkSurfaceCapabilitiesKHR        get_surface_capabilities() const;

            // Find present queue family
            uint32_t find_present_queue_family();
    };
} // namespace vulkan_engine::vulkan