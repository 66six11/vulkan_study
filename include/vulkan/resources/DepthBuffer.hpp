#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace vulkan_engine::vulkan
{
    class DepthBuffer
    {
        public:
            DepthBuffer(
                std::shared_ptr<DeviceManager> device,
                uint32_t                       width,
                uint32_t                       height);
            ~DepthBuffer();

            // Non-copyable
            DepthBuffer(const DepthBuffer&)            = delete;
            DepthBuffer& operator=(const DepthBuffer&) = delete;

            // Movable
            DepthBuffer(DepthBuffer&& other) noexcept;
            DepthBuffer& operator=(DepthBuffer&& other) noexcept;

            VkImage        image() const { return image_; }
            VkImageView    view() const { return view_; }
            VkDeviceMemory memory() const { return memory_; }
            VkFormat       format() const { return format_; }

            static VkFormat find_depth_format(std::shared_ptr<DeviceManager> device);

        private:
            std::shared_ptr<DeviceManager> device_;
            VkImage                        image_  = VK_NULL_HANDLE;
            VkImageView                    view_   = VK_NULL_HANDLE;
            VkDeviceMemory                 memory_ = VK_NULL_HANDLE;
            VkFormat                       format_ = VK_FORMAT_UNDEFINED;
            uint32_t                       width_  = 0;
            uint32_t                       height_ = 0;

            void create_image();
            void create_view();
    };
} // namespace vulkan_engine::vulkan