#include "vulkan/sync/Synchronization.hpp"
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

        if (vkCreateFence(device_->device(), &fence_info, nullptr, &fence_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fence");
        }
    }

    Fence::~Fence()
    {
        if (fence_ != VK_NULL_HANDLE)
        {
            vkDestroyFence(device_->device(), fence_, nullptr);
        }
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

        if (vkCreateSemaphore(device_->device(), &semaphore_info, nullptr, &semaphore_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphore");
        }
    }

    Semaphore::~Semaphore()
    {
        if (semaphore_ != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device_->device(), semaphore_, nullptr);
        }
    }

    // TimelineSemaphore implementation
    TimelineSemaphore::TimelineSemaphore(std::shared_ptr<DeviceManager> device, uint64_t /*initial_value*/)
        : Semaphore(std::move(device))
    {
        // Note: This would need proper timeline semaphore creation
        // For now, using regular semaphore as placeholder
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(device_->device(), &semaphore_info, nullptr, &semaphore_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create timeline semaphore");
        }
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

        if (vkCreateEvent(device_->device(), &event_info, nullptr, &event_) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create event");
        }
    }

    Event::~Event()
    {
        if (event_ != VK_NULL_HANDLE)
        {
            vkDestroyEvent(device_->device(), event_, nullptr);
        }
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
} // namespace vulkan_engine::vulkan