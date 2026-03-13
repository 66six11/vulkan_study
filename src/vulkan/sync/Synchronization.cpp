#include "vulkan/sync/Synchronization.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <stdexcept>

namespace vulkan_engine::vulkan
{
    // Fence implementation
    Fence::Fence(std::shared_ptr<DeviceManager> device, bool signaled)
        : device_(std::move(device))
    {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled)
        {
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkResult result = vkCreateFence(device_->device(), &fence_info, nullptr, &fence_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create fence", __FILE__, __LINE__);
        }
    }

    Fence::~Fence()
    {
        if (fence_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyFence(device_->device(), fence_, nullptr);
        }
    }

    Fence::Fence(Fence&& other) noexcept
        : device_(std::move(other.device_))
        , fence_(other.fence_)
    {
        other.fence_ = VK_NULL_HANDLE;
    }

    Fence& Fence::operator=(Fence&& other) noexcept
    {
        if (this != &other)
        {
            if (fence_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyFence(device_->device(), fence_, nullptr);
            }

            device_      = std::move(other.device_);
            fence_       = other.fence_;
            other.fence_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Fence::wait(uint64_t timeout)
    {
        vkWaitForFences(device_->device(), 1, &fence_, VK_TRUE, timeout);
    }

    void Fence::reset()
    {
        vkResetFences(device_->device(), 1, &fence_);
    }

    bool Fence::is_signaled() const
    {
        return vkGetFenceStatus(device_->device(), fence_) == VK_SUCCESS;
    }

    void Fence::wait_and_reset(uint64_t timeout)
    {
        wait(timeout);
        reset();
    }

    // Semaphore implementation
    Semaphore::Semaphore(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult result = vkCreateSemaphore(device_->device(), &semaphore_info, nullptr, &semaphore_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create semaphore", __FILE__, __LINE__);
        }
    }

    Semaphore::~Semaphore()
    {
        if (semaphore_ != VK_NULL_HANDLE && device_)
        {
            vkDestroySemaphore(device_->device(), semaphore_, nullptr);
        }
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept
        : device_(std::move(other.device_))
        , semaphore_(other.semaphore_)
    {
        other.semaphore_ = VK_NULL_HANDLE;
    }

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
    {
        if (this != &other)
        {
            if (semaphore_ != VK_NULL_HANDLE && device_)
            {
                vkDestroySemaphore(device_->device(), semaphore_, nullptr);
            }

            device_          = std::move(other.device_);
            semaphore_       = other.semaphore_;
            other.semaphore_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    // TimelineSemaphore implementation
    TimelineSemaphore::TimelineSemaphore(std::shared_ptr<DeviceManager> device, uint64_t /*initial_value*/)
        : Semaphore(std::move(device))
    {
        // Note: Proper timeline semaphore requires Vulkan 1.2+
        // For now, using regular semaphore as placeholder
        // The base Semaphore class already creates the semaphore
    }

    void TimelineSemaphore::signal(uint64_t /*value*/)
    {
        // Placeholder: Timeline semaphore signaling requires Vulkan 1.2+
        // vkSignalSemaphore(device_->device(), &signal_info);
    }

    void TimelineSemaphore::wait(uint64_t /*value*/, uint64_t /*timeout*/)
    {
        // Placeholder: Timeline semaphore wait requires Vulkan 1.2+
        // vkWaitSemaphores(device_->device(), &wait_info, timeout);
    }

    uint64_t TimelineSemaphore::get_value() const
    {
        // Placeholder: Timeline semaphore counter value requires Vulkan 1.2+
        // vkGetSemaphoreCounterValue(device_->device(), semaphore_, &value);
        return 0;
    }

    // Event implementation
    Event::Event(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
        VkEventCreateInfo event_info{};
        event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

        VkResult result = vkCreateEvent(device_->device(), &event_info, nullptr, &event_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create event", __FILE__, __LINE__);
        }
    }

    Event::~Event()
    {
        if (event_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyEvent(device_->device(), event_, nullptr);
        }
    }

    Event::Event(Event&& other) noexcept
        : device_(std::move(other.device_))
        , event_(other.event_)
    {
        other.event_ = VK_NULL_HANDLE;
    }

    Event& Event::operator=(Event&& other) noexcept
    {
        if (this != &other)
        {
            if (event_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyEvent(device_->device(), event_, nullptr);
            }

            device_      = std::move(other.device_);
            event_       = other.event_;
            other.event_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Event::set()
    {
        vkSetEvent(device_->device(), event_);
    }

    void Event::reset()
    {
        vkResetEvent(device_->device(), event_);
    }

    bool Event::is_set() const
    {
        return vkGetEventStatus(device_->device(), event_) == VK_EVENT_SET;
    }

    // SynchronizationManager implementation
    SynchronizationManager::SynchronizationManager(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    SynchronizationManager::~SynchronizationManager() = default;

    std::shared_ptr<Fence> SynchronizationManager::create_fence(bool signaled)
    {
        return std::make_shared < Fence > (device_, signaled);
    }

    std::shared_ptr<Semaphore> SynchronizationManager::create_semaphore()
    {
        return std::make_shared < Semaphore > (device_);
    }

    std::shared_ptr<TimelineSemaphore> SynchronizationManager::create_timeline_semaphore(uint64_t initial_value)
    {
        return std::make_shared < TimelineSemaphore > (device_, initial_value);
    }

    std::shared_ptr<Event> SynchronizationManager::create_event()
    {
        return std::make_shared < Event > (device_);
    }

    void SynchronizationManager::wait_for_fences(
        const std::vector<std::shared_ptr<Fence>>& fences,
        bool                                       wait_all,
        uint64_t                                   timeout)
    {
        std::vector<VkFence> vk_fences;
        vk_fences.reserve(fences.size());
        for (const auto& fence : fences)
        {
            vk_fences.push_back(fence->handle());
        }

        vkWaitForFences(device_->device(),
                        static_cast<uint32_t>(vk_fences.size()),
                        vk_fences.data(),
                        wait_all ? VK_TRUE : VK_FALSE,
                        timeout);
    }

    void SynchronizationManager::reset_fences(const std::vector<std::shared_ptr<Fence>>& fences)
    {
        std::vector<VkFence> vk_fences;
        vk_fences.reserve(fences.size());
        for (const auto& fence : fences)
        {
            vk_fences.push_back(fence->handle());
        }

        vkResetFences(device_->device(), static_cast<uint32_t>(vk_fences.size()), vk_fences.data());
    }

    void SynchronizationManager::submit_with_sync(
        VkQueue                                        queue,
        VkCommandBuffer                                cmd,
        const std::vector<std::shared_ptr<Semaphore>>& wait_semaphores,
        const std::vector<std::shared_ptr<Semaphore>>& signal_semaphores,
        std::shared_ptr<Fence>                         fence)
    {
        std::vector<VkSemaphore>          vk_wait_semaphores;
        std::vector<VkPipelineStageFlags> wait_stages;
        for (const auto& sem : wait_semaphores)
        {
            vk_wait_semaphores.push_back(sem->handle());
            wait_stages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }

        std::vector<VkSemaphore> vk_signal_semaphores;
        for (const auto& sem : signal_semaphores)
        {
            vk_signal_semaphores.push_back(sem->handle());
        }

        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount   = static_cast<uint32_t>(vk_wait_semaphores.size());
        submit_info.pWaitSemaphores      = vk_wait_semaphores.data();
        submit_info.pWaitDstStageMask    = wait_stages.data();
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &cmd;
        submit_info.signalSemaphoreCount = static_cast<uint32_t>(vk_signal_semaphores.size());
        submit_info.pSignalSemaphores    = vk_signal_semaphores.data();

        VkFence fence_handle = fence ? fence->handle() : VK_NULL_HANDLE;
        vkQueueSubmit(queue, 1, &submit_info, fence_handle);
    }

    // FrameSyncManager implementation
    FrameSyncManager::FrameSyncManager(std::shared_ptr<DeviceManager> device, uint32_t frame_count)
        : device_(std::move(device))
        , frame_count_(frame_count)
    {
        fences_.reserve(frame_count);
        image_available_semaphores_.reserve(frame_count);
        render_finished_semaphores_.reserve(frame_count);

        for (uint32_t i = 0; i < frame_count; ++i)
        {
            fences_.push_back(std::make_unique < Fence > (device_, true)); // Start signaled
            image_available_semaphores_.push_back(std::make_unique < Semaphore > (device_));
            render_finished_semaphores_.push_back(std::make_unique < Semaphore > (device_));
        }
    }
} // namespace vulkan_engine::vulkan