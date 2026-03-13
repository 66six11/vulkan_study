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

            // Movable
            Fence(Fence&& other) noexcept;
            Fence& operator=(Fence&& other) noexcept;

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

            // Movable
            Semaphore(Semaphore&& other) noexcept;
            Semaphore& operator=(Semaphore&& other) noexcept;

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

            // Movable
            Event(Event&& other) noexcept;
            Event& operator=(Event&& other) noexcept;

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

    // Frame synchronization manager for double/triple buffering
    class FrameSyncManager
    {
        public:
            FrameSyncManager(std::shared_ptr<DeviceManager> device, uint32_t frame_count);
            ~FrameSyncManager() = default;

            // Non-copyable
            FrameSyncManager(const FrameSyncManager&)            = delete;
            FrameSyncManager& operator=(const FrameSyncManager&) = delete;

            // Get sync objects for current frame
            uint32_t get_current_frame() const { return current_frame_; }
            void     next_frame() { current_frame_ = (current_frame_ + 1) % frame_count_; }

            // Per-frame sync objects
            Fence&     get_fence(uint32_t frame) { return *fences_[frame]; }
            Fence&     get_current_fence() { return *fences_[current_frame_]; }
            Semaphore& get_image_available_semaphore(uint32_t frame) { return *image_available_semaphores_[frame]; }
            Semaphore& get_current_image_available_semaphore() { return *image_available_semaphores_[current_frame_]; }
            Semaphore& get_render_finished_semaphore(uint32_t frame) { return *render_finished_semaphores_[frame]; }
            Semaphore& get_current_render_finished_semaphore() { return *render_finished_semaphores_[current_frame_]; }

            // Convenience methods
            void wait_for_current_frame_fence(uint64_t timeout = UINT64_MAX)
            {
                fences_[current_frame_]->wait(timeout);
            }

            void reset_current_frame_fence() { fences_[current_frame_]->reset(); }

            void wait_and_reset_current_fence(uint64_t timeout = UINT64_MAX)
            {
                fences_[current_frame_]->wait_and_reset(timeout);
            }

            uint32_t frame_count() const { return frame_count_; }

        private:
            std::shared_ptr<DeviceManager>          device_;
            uint32_t                                frame_count_;
            uint32_t                                current_frame_ = 0;
            std::vector<std::unique_ptr<Fence>>     fences_;
            std::vector<std::unique_ptr<Semaphore>> image_available_semaphores_;
            std::vector<std::unique_ptr<Semaphore>> render_finished_semaphores_;
    };
} // namespace vulkan_engine::vulkan