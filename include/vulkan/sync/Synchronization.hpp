#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace vulkan_engine::vulkan
{
    class Fence
    {
        public:
            Fence(std::shared_ptr<DeviceManager> device, bool signaled = false);
            ~Fence();

            // Non-copyable
            Fence(const Fence&)            = delete;
            Fence& operator=(const Fence&) = delete;

            void wait(uint64_t timeout = UINT64_MAX);
            void reset();
            bool is_signaled() const;
            void wait_and_reset(uint64_t timeout = UINT64_MAX);

            VkFence handle() const { return fence_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkFence                        fence_ = VK_NULL_HANDLE;
    };

    class Semaphore
    {
        public:
            explicit Semaphore(std::shared_ptr<DeviceManager> device);
            ~Semaphore();

            // Non-copyable
            Semaphore(const Semaphore&)            = delete;
            Semaphore& operator=(const Semaphore&) = delete;

            VkSemaphore handle() const { return semaphore_; }

        protected:
            std::shared_ptr<DeviceManager> device_;
            VkSemaphore                    semaphore_ = VK_NULL_HANDLE;
    };

    class TimelineSemaphore : public Semaphore
    {
        public:
            TimelineSemaphore(std::shared_ptr<DeviceManager> device, uint64_t initial_value = 0);
            ~TimelineSemaphore() = default;

            void     signal(uint64_t value);
            void     wait(uint64_t value, uint64_t timeout = UINT64_MAX);
            uint64_t get_value() const;
    };

    class Event
    {
        public:
            explicit Event(std::shared_ptr<DeviceManager> device);
            ~Event();

            // Non-copyable
            Event(const Event&)            = delete;
            Event& operator=(const Event&) = delete;

            void set();
            void reset();
            bool is_set() const;

            VkEvent handle() const { return event_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkEvent                        event_ = VK_NULL_HANDLE;
    };

    class SynchronizationManager
    {
        public:
            explicit SynchronizationManager(std::shared_ptr<DeviceManager> device);
            ~SynchronizationManager();

            std::shared_ptr<Fence>             create_fence(bool signaled = false);
            std::shared_ptr<Semaphore>         create_semaphore();
            std::shared_ptr<TimelineSemaphore> create_timeline_semaphore(uint64_t initial_value = 0);
            std::shared_ptr<Event>             create_event();

            void wait_for_fences(
                const std::vector<std::shared_ptr<Fence>>& fences,
                bool                                       wait_all = true,
                uint64_t                                   timeout  = UINT64_MAX);
            void reset_fences(const std::vector<std::shared_ptr<Fence>>& fences);

            void submit_with_sync(
                VkQueue                                        queue,
                VkCommandBuffer                                cmd,
                const std::vector<std::shared_ptr<Semaphore>>& wait_semaphores   = {},
                const std::vector<std::shared_ptr<Semaphore>>& signal_semaphores = {},
                std::shared_ptr<Fence>                         fence             = nullptr);

        private:
            std::shared_ptr<DeviceManager> device_;
    };
} // namespace vulkan_engine::vulkan