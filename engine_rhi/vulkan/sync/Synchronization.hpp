#pragma once

// Compatibility layer: Frame synchronization

#include "engine/rhi/Device.hpp"
#include "engine/rhi/SwapChain.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace engine::vulkan
{
    // Frame synchronization data
    struct FrameSync
    {
        rhi::SemaphoreHandle imageAvailable;
        rhi::SemaphoreHandle renderFinished;
        rhi::FenceHandle     inFlight;

        [[nodiscard]] VkSemaphore image_available_semaphore() const
        {
            return imageAvailable ? imageAvailable->nativeHandle() : VK_NULL_HANDLE;
        }

        [[nodiscard]] VkSemaphore render_finished_semaphore() const
        {
            return renderFinished ? renderFinished->nativeHandle() : VK_NULL_HANDLE;
        }

        [[nodiscard]] VkFence in_flight_fence() const
        {
            return inFlight ? inFlight->nativeHandle() : VK_NULL_HANDLE;
        }
    };

    // FrameSyncManager manages per-frame synchronization primitives
    class FrameSyncManager
    {
        public:
            FrameSyncManager() = default;

            FrameSyncManager(std::shared_ptr<rhi::Device> device, uint32_t frames_in_flight)
                : device_(std::move(device))
                , framesInFlight_(frames_in_flight)
            {
                initialize();
            }

            ~FrameSyncManager()
            {
                shutdown();
            }

            void initialize()
            {
                if (!device_ || !frames_.empty()) return;

                frames_.resize(framesInFlight_);
                for (auto& frame : frames_)
                {
                    auto semResult1  = device_->createSemaphore();
                    auto semResult2  = device_->createSemaphore();
                    auto fenceResult = device_->createFence(true); // signaled

                    if (semResult1.has_value()) frame.imageAvailable = semResult1.value();
                    if (semResult2.has_value()) frame.renderFinished = semResult2.value();
                    if (fenceResult.has_value()) frame.inFlight = fenceResult.value();
                }
            }

            void shutdown()
            {
                if (!device_) return;
                device_->waitIdle();

                for (auto& frame : frames_)
                {
                    if (frame.imageAvailable) device_->destroySemaphore(frame.imageAvailable);
                    if (frame.renderFinished) device_->destroySemaphore(frame.renderFinished);
                    if (frame.inFlight) device_->destroyFence(frame.inFlight);
                }
                frames_.clear();
                renderFinishedSemaphores_.clear();
            }

            // Get frame sync for current frame
            [[nodiscard]] FrameSync& frame(uint32_t index)
            {
                return frames_[index % framesInFlight_];
            }

            [[nodiscard]] const FrameSync& frame(uint32_t index) const
            {
                return frames_[index % framesInFlight_];
            }

            // Wait for frame fence
            void wait_for_frame(uint32_t index)
            {
                auto& f = frame(index);
                if (f.inFlight)
                {
                    device_->waitForFence(f.inFlight);
                }
            }

            // Reset frame fence
            void reset_frame_fence(uint32_t index)
            {
                auto& f = frame(index);
                if (f.inFlight)
                {
                    device_->resetFence(f.inFlight);
                }
            }

            // Additional render finished semaphores for swap chain images
            void resize_render_finished_semaphores(uint32_t count)
            {
                renderFinishedSemaphores_.clear();
                renderFinishedSemaphores_.resize(count);
                for (auto& sem : renderFinishedSemaphores_)
                {
                    auto result = device_->createSemaphore();
                    if (result.has_value())
                    {
                        sem = result.value();
                    }
                }
            }

            [[nodiscard]] rhi::SemaphoreHandle render_finished_semaphore(uint32_t index) const
            {
                if (index < renderFinishedSemaphores_.size())
                {
                    return renderFinishedSemaphores_[index];
                }
                return nullptr;
            }

            [[nodiscard]] uint32_t frames_in_flight() const { return framesInFlight_; }

        private:
            std::shared_ptr<rhi::Device>      device_;
            uint32_t                          framesInFlight_ = 2;
            std::vector<FrameSync>            frames_;
            std::vector<rhi::SemaphoreHandle> renderFinishedSemaphores_;
    };
} // namespace engine::vulkan