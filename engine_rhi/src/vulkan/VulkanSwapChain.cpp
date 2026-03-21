#include "engine/rhi/SwapChain.hpp"
#include <vulkan/vulkan.h>

namespace engine::rhi
{
    // SwapChain implementation
    SwapChain::~SwapChain()
    {
        release();
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , surface_(other.surface_)
        , presentQueue_(other.presentQueue_)
        , images_(std::move(other.images_))
        , imageViews_(std::move(other.imageViews_))
        , extent_(other.extent_)
        , format_(other.format_)
        , currentImageIndex_(other.currentImageIndex_)
    {
        other.handle_       = nullptr;
        other.device_       = nullptr;
        other.surface_      = nullptr;
        other.presentQueue_ = nullptr;
    }

    SwapChain& SwapChain::operator=(SwapChain&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_             = other.handle_;
            device_             = other.device_;
            surface_            = other.surface_;
            presentQueue_       = other.presentQueue_;
            images_             = std::move(other.images_);
            imageViews_         = std::move(other.imageViews_);
            extent_             = other.extent_;
            format_             = other.format_;
            currentImageIndex_  = other.currentImageIndex_;
            other.handle_       = nullptr;
            other.device_       = nullptr;
            other.surface_      = nullptr;
            other.presentQueue_ = nullptr;
        }
        return *this;
    }

    SwapChain::SwapChain(const InternalData& data)
        : handle_(data.swapchain)
        , device_(data.device)
        , surface_(data.surface)
        , presentQueue_(data.presentQueue)
        , images_(data.images)
        , imageViews_(data.imageViews)
        , extent_(data.extent)
        , format_(data.format)
    {
    }

    void SwapChain::release()
    {
        if (device_)
        {
            // Images are owned by swapchain, just clear handles
            images_.clear();
            imageViews_.clear();

            if (handle_)
            {
                vkDestroySwapchainKHR(device_, handle_, nullptr);
                handle_ = nullptr;
            }

            if (surface_)
            {
                // Surface is destroyed separately (needs instance)
                // For now, we assume Device manages this
                surface_ = nullptr;
            }

            device_       = nullptr;
            presentQueue_ = nullptr;
        }
    }

    ResultValue<uint32_t> SwapChain::acquireNextImage(
        SemaphoreHandle signalSemaphore,
        FenceHandle     signalFence,
        uint64_t        timeout)
    {
        if (!handle_)
        {
            return std::unexpected(Result::Error_InvalidParameter);
        }

        VkSemaphore semaphore = signalSemaphore ? signalSemaphore->nativeHandle() : nullptr;
        VkFence     fence     = signalFence ? signalFence->nativeHandle() : nullptr;

        uint32_t imageIndex = 0;
        VkResult result     = vkAcquireNextImageKHR(device_,
                                                    handle_,
                                                    timeout,
                                                    semaphore,
                                                    fence,
                                                    &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return std::unexpected(Result::Error_OutOfDate);
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            // Can still present, but should recreate soon
            currentImageIndex_ = imageIndex;
            return imageIndex;
        }
        else if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_DeviceLost);
        }

        currentImageIndex_ = imageIndex;
        return imageIndex;
    }

    Result SwapChain::resize(const Extent2D& newExtent)
    {
        if (!handle_)
        {
            return Result::Error_InvalidParameter;
        }

        // Resize is handled by recreating swapchain
        // This would need Device reference to recreate
        // For now, return success and let caller handle recreation
        extent_ = newExtent;
        return Result::Success;
    }

    Result SwapChain::present(const std::vector<SemaphoreHandle>& waitSemaphores)
    {
        if (!handle_ || !presentQueue_)
        {
            return Result::Error_InvalidParameter;
        }

        std::vector<VkSemaphore> semaphores;
        semaphores.reserve(waitSemaphores.size());
        for (const auto& sem : waitSemaphores)
        {
            if (sem) semaphores.push_back(sem->nativeHandle());
        }

        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
        presentInfo.pWaitSemaphores    = semaphores.empty() ? nullptr : semaphores.data();
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &handle_;
        presentInfo.pImageIndices      = &currentImageIndex_;

        VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            return Result::Error_OutOfDate;
        }
        else if (result != VK_SUCCESS)
        {
            return Result::Error_DeviceLost;
        }

        return Result::Success;
    }

    // Semaphore implementation
    Semaphore::~Semaphore()
    {
        release();
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    Semaphore::Semaphore(const InternalData& data)
        : handle_(data.semaphore)
        , device_(data.device)
    {
    }

    void Semaphore::release()
    {
        if (handle_ && device_)
        {
            vkDestroySemaphore(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }

    // Fence implementation
    Fence::~Fence()
    {
        release();
    }

    Fence::Fence(Fence&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    Fence& Fence::operator=(Fence&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    Fence::Fence(const InternalData& data)
        : handle_(data.fence)
        , device_(data.device)
    {
    }

    void Fence::release()
    {
        if (handle_ && device_)
        {
            vkDestroyFence(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }

    Result Fence::wait(uint64_t timeout)
    {
        if (!handle_) return Result::Error_InvalidParameter;

        VkResult result = vkWaitForFences(device_, 1, &handle_, VK_TRUE, timeout);

        if (result == VK_TIMEOUT)
        {
            return Result::Error_Timeout;
        }
        else if (result != VK_SUCCESS)
        {
            return Result::Error_DeviceLost;
        }

        return Result::Success;
    }

    Result Fence::reset()
    {
        if (!handle_) return Result::Error_InvalidParameter;

        VkResult result = vkResetFences(device_, 1, &handle_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_DeviceLost;
        }

        return Result::Success;
    }

    ResultValue<bool> Fence::isSignaled()
    {
        if (!handle_) return std::unexpected(Result::Error_InvalidParameter);

        VkResult result = vkGetFenceStatus(device_, handle_);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else if (result == VK_NOT_READY)
        {
            return false;
        }
        else
        {
            return std::unexpected(Result::Error_DeviceLost);
        }
    }
} // namespace engine::rhi
