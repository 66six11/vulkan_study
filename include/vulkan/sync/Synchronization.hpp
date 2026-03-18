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

            Fence(const Fence&)            = delete;
            Fence& operator=(const Fence&) = delete;

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

            Semaphore(const Semaphore&)            = delete;
            Semaphore& operator=(const Semaphore&) = delete;

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

            Event(const Event&)            = delete;
            Event& operator=(const Event&) = delete;

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

    /**
     * @brief Simplified frame synchronization manager
     * 
     * Pure per-frame architecture:
     * - All resources indexed by frame index (0..max_frames_in_flight-1)
     * - No per-image arrays to avoid confusion
     * - Simple, predictable, easy to debug
     */
    class FrameSyncManager
    {
        public:
            /**
             * @brief Construct frame sync manager
             * 
             * @param device Vulkan device manager
             * @param frame_count Number of frames in flight (typically 2 or 3)
             */
            FrameSyncManager(
                std::shared_ptr<DeviceManager> device,
                uint32_t                       frame_count);
            ~FrameSyncManager() = default;

            FrameSyncManager(const FrameSyncManager&)            = delete;
            FrameSyncManager& operator=(const FrameSyncManager&) = delete;

            // Frame management
            uint32_t current_frame() const { return current_frame_; }

            /**
             * @brief Advance to next frame
             * @return New frame index
             */
            uint32_t advance_frame()
            {
                current_frame_ = (current_frame_ + 1) % frame_count_;
                return current_frame_;
            }

            // Per-frame: CPU-GPU synchronization (for command buffer recycling)
            Fence& get_frame_fence(uint32_t frame) { return *frame_fences_[frame]; }
            Fence& get_current_frame_fence() { return *frame_fences_[current_frame_]; }

            /**
             * @brief Wait for frame fence and reset for reuse
             * 
             * This ensures GPU has finished all work for this frame slot
             * and the command buffers can be safely reset.
             */
            void wait_and_reset_frame_fence(uint32_t frame, uint64_t timeout = UINT64_MAX);
            void wait_and_reset_current_frame_fence(uint64_t timeout = UINT64_MAX);

            // Per-frame: GPU-GPU synchronization
            Semaphore& get_acquire_semaphore(uint32_t frame) { return *acquire_semaphores_[frame]; }
            Semaphore& get_current_acquire_semaphore() { return *acquire_semaphores_[current_frame_]; }

            Semaphore& get_scene_finished_semaphore(uint32_t frame) { return *scene_finished_semaphores_[frame]; }
            Semaphore& get_current_scene_finished_semaphore() { return *scene_finished_semaphores_[current_frame_]; }

            Semaphore& get_render_finished_semaphore(uint32_t frame) { return *render_finished_semaphores_[frame]; }
            Semaphore& get_current_render_finished_semaphore() { return *render_finished_semaphores_[current_frame_]; }

            uint32_t frame_count() const { return frame_count_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            uint32_t                       frame_count_;
            uint32_t                       current_frame_ = 0;

            // Per-frame: CPU-GPU synchronization (fence for command buffer lifecycle)
            std::vector<std::unique_ptr<Fence>> frame_fences_;

            // Per-frame: GPU-GPU synchronization
            std::vector<std::unique_ptr<Semaphore>> acquire_semaphores_;         // vkAcquireNextImageKHR
            std::vector<std::unique_ptr<Semaphore>> scene_finished_semaphores_;  // Scene -> ImGui dependency
            std::vector<std::unique_ptr<Semaphore>> render_finished_semaphores_; // vkQueuePresentKHR
    };
} // namespace vulkan_engine::vulkan