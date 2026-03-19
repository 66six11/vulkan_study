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
     * Hybrid architecture:
     * - Per-frame: Fence (for command buffer lifecycle management)
     * - Per-image: Semaphore (for swapchain image synchronization)
     * 
     * This design follows Vulkan best practices:
     * - Each swapchain image gets its own acquire/render_finished semaphore
     * - Fence tracks per-frame CPU-GPU synchronization
     */
    class FrameSyncManager
    {
        public:
            /**
             * @brief Construct frame sync manager
             * 
             * @param device Vulkan device manager
             * @param max_frames_in_flight Maximum frames in flight (typically 2 or 3)
             */
            FrameSyncManager(
                std::shared_ptr<DeviceManager> device,
                uint32_t                       max_frames_in_flight);
            ~FrameSyncManager() = default;

            FrameSyncManager(const FrameSyncManager&)            = delete;
            FrameSyncManager& operator=(const FrameSyncManager&) = delete;

            // Frame management (for command buffer recycling)
            uint32_t current_frame() const { return current_frame_; }

            /**
             * @brief Advance to next frame
             * @return New frame index
             */
            uint32_t advance_frame()
            {
                current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
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

            // Per-frame: acquire semaphore for vkAcquireNextImageKHR
            // (must be per-frame because we don't know image index until after acquire)
            Semaphore& get_current_acquire_semaphore() { return *acquire_semaphores_[current_frame_]; }

            // Per-image: render finished semaphore for vkQueuePresentKHR
            // (must be per-image because swapchain may present images out of order)
            Semaphore& get_render_finished_semaphore(uint32_t image_index) { return *render_finished_semaphores_[image_index]; }

            // Resize per-image semaphores when swapchain is recreated
            void resize_render_finished_semaphores(uint32_t image_count);

            // Per-frame: Scene rendering synchronization
            Semaphore& get_scene_finished_semaphore(uint32_t frame) { return *scene_finished_semaphores_[frame]; }
            Semaphore& get_current_scene_finished_semaphore() { return *scene_finished_semaphores_[current_frame_]; }

            uint32_t max_frames_in_flight() const { return max_frames_in_flight_; }

            /**
             * @brief Resize semaphore arrays for swapchain image count
             * Call this after swapchain recreation when image count changes
             */


        private:
            std::shared_ptr<DeviceManager> device_;
            uint32_t                       max_frames_in_flight_;
            uint32_t                       current_frame_ = 0;

            // Per-frame: CPU-GPU synchronization (fence for command buffer lifecycle)
            std::vector<std::unique_ptr<Fence>> frame_fences_;

            // Per-frame: Scene -> ImGui dependency
            std::vector<std::unique_ptr<Semaphore>> scene_finished_semaphores_;

            // Per-image: GPU-GPU synchronization for swapchain
            // These are indexed by swapchain image index
            std::vector<std::unique_ptr<Semaphore>> acquire_semaphores_;         // vkAcquireNextImageKHR
            std::vector<std::unique_ptr<Semaphore>> render_finished_semaphores_; // vkQueuePresentKHR
    };
} // namespace vulkan_engine::vulkan