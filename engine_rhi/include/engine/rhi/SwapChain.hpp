#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Core.hpp"
#include "Texture.hpp"

namespace engine::rhi
{
    class Fence;
    class Semaphore;
    using SemaphoreHandle = std::shared_ptr<Semaphore>;
    using FenceHandle     = std::shared_ptr<Fence>;
    // Forward declarations
    class Device;

    // Swap chain description
    struct SwapChainDesc
    {
        void*    windowHandle = nullptr; // Platform-specific window handle
        Extent2D extent       = {1280, 720};
        Format   format       = Format::B8G8R8A8_UNORM;
        uint32_t imageCount   = 2; // Double buffering by default
        bool     vsync        = true;
        bool     fullscreen   = false;
    };

    // Swap chain class
    class SwapChain
    {
        public:
            SwapChain() = default;
            ~SwapChain();

            SwapChain(const SwapChain&)            = delete;
            SwapChain& operator=(const SwapChain&) = delete;

            SwapChain(SwapChain&& other) noexcept;
            SwapChain& operator=(SwapChain&& other) noexcept;

            [[nodiscard]] bool           isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkSwapchainKHR nativeHandle() const noexcept { return handle_; }

            // Properties
            [[nodiscard]] const Extent2D& extent() const noexcept { return extent_; }
            [[nodiscard]] Format          format() const noexcept { return format_; }
            [[nodiscard]] uint32_t        imageCount() const noexcept { return static_cast<uint32_t>(images_.size()); }

            // Current image
            [[nodiscard]] uint32_t currentImageIndex() const noexcept { return currentImageIndex_; }

            [[nodiscard]] TextureHandle currentImage() const
            {
                if (currentImageIndex_ < images_.size())
                {
                    return images_[currentImageIndex_];
                }
                return nullptr;
            }

            [[nodiscard]] TextureViewHandle currentImageView() const
            {
                if (currentImageIndex_ < imageViews_.size())
                {
                    return imageViews_[currentImageIndex_];
                }
                return nullptr;
            }

            // All images
            [[nodiscard]] const std::vector<TextureHandle>&     images() const { return images_; }
            [[nodiscard]] const std::vector<TextureViewHandle>& imageViews() const { return imageViews_; }

            // Operations
            [[nodiscard]] ResultValue<uint32_t> acquireNextImage(
                SemaphoreHandle signalSemaphore,
                FenceHandle     signalFence = nullptr,
                uint64_t        timeout     = UINT64_MAX);

            // Resize
            [[nodiscard]] Result resize(const Extent2D& newExtent);

            // Present
            [[nodiscard]] Result present(const std::vector<SemaphoreHandle>& waitSemaphores);

            // Internal construction
            struct InternalData
            {
                VkSwapchainKHR                 swapchain    = nullptr;
                VkDevice                       device       = nullptr;
                VkSurfaceKHR                   surface      = nullptr;
                VkQueue                        presentQueue = nullptr;
                std::vector<TextureHandle>     images;
                std::vector<TextureViewHandle> imageViews;
                Extent2D                       extent;
                Format                         format;
            };

            explicit SwapChain(const InternalData& data);
            void     release();

        private:
            VkSwapchainKHR handle_       = nullptr;
            VkDevice       device_       = nullptr;
            VkSurfaceKHR   surface_      = nullptr;
            VkQueue        presentQueue_ = nullptr;

            std::vector<TextureHandle>     images_;
            std::vector<TextureViewHandle> imageViews_;

            Extent2D extent_;
            Format   format_;
            uint32_t currentImageIndex_ = 0;
    };

    using SwapChainHandle = std::shared_ptr<SwapChain>;

    // Semaphore class
    class Semaphore
    {
        public:
            Semaphore() = default;
            ~Semaphore();

            Semaphore(const Semaphore&)            = delete;
            Semaphore& operator=(const Semaphore&) = delete;

            Semaphore(Semaphore&& other) noexcept;
            Semaphore& operator=(Semaphore&& other) noexcept;

            [[nodiscard]] bool        isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkSemaphore nativeHandle() const noexcept { return handle_; }

            struct InternalData
            {
                VkSemaphore semaphore = nullptr;
                VkDevice    device    = nullptr;
            };

            explicit Semaphore(const InternalData& data);
            void     release();

        private:
            VkSemaphore handle_ = nullptr;
            VkDevice    device_ = nullptr;
    };


    // Fence class
    class Fence
    {
        public:
            Fence() = default;
            ~Fence();

            Fence(const Fence&)            = delete;
            Fence& operator=(const Fence&) = delete;

            Fence(Fence&& other) noexcept;
            Fence& operator=(Fence&& other) noexcept;

            [[nodiscard]] bool    isValid() const noexcept { return handle_ != nullptr; }
            [[nodiscard]] VkFence nativeHandle() const noexcept { return handle_; }

            [[nodiscard]] Result            wait(uint64_t timeout = UINT64_MAX);
            [[nodiscard]] Result            reset();
            [[nodiscard]] ResultValue<bool> isSignaled();

            struct InternalData
            {
                VkFence  fence  = nullptr;
                VkDevice device = nullptr;
            };

            explicit Fence(const InternalData& data);
            void     release();

        private:
            VkFence  handle_ = nullptr;
            VkDevice device_ = nullptr;
    };
} // namespace engine::rhi
