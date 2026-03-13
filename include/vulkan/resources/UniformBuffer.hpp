#pragma once

#include "vulkan/device/Device.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <cstring>

namespace vulkan_engine::vulkan
{
    // Per-frame uniform buffer for dynamic data
    template <typename T> class UniformBuffer
    {
        public:
            UniformBuffer(std::shared_ptr<DeviceManager> device, uint32_t frame_count = 1)
                : device_(std::move(device))
                , frame_count_(frame_count)
                , current_frame_(0)
            {
                VkDeviceSize buffer_size = sizeof(T);

                buffers_.resize(frame_count);
                memories_.resize(frame_count);
                mapped_data_.resize(frame_count);

                for (uint32_t i = 0; i < frame_count; ++i)
                {
                    // Create buffer
                    VkBufferCreateInfo buffer_info{};
                    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    buffer_info.size        = buffer_size;
                    buffer_info.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    VkResult result = vkCreateBuffer(device_->device(), &buffer_info, nullptr, &buffers_[i]);
                    if (result != VK_SUCCESS)
                    {
                        throw VulkanError(result, "Failed to create uniform buffer", __FILE__, __LINE__);
                    }

                    // Get memory requirements
                    VkMemoryRequirements mem_requirements;
                    vkGetBufferMemoryRequirements(device_->device(), buffers_[i], &mem_requirements);

                    // Allocate memory (host visible for dynamic updates)
                    VkMemoryAllocateInfo alloc_info{};
                    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    alloc_info.allocationSize  = mem_requirements.size;
                    alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                                           mem_requirements.memoryTypeBits,
                                                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                    result = vkAllocateMemory(device_->device(), &alloc_info, nullptr, &memories_[i]);
                    if (result != VK_SUCCESS)
                    {
                        throw VulkanError(result, "Failed to allocate uniform buffer memory", __FILE__, __LINE__);
                    }

                    // Bind memory
                    vkBindBufferMemory(device_->device(), buffers_[i], memories_[i], 0);

                    // Map memory persistently
                    vkMapMemory(device_->device(), memories_[i], 0, buffer_size, 0, &mapped_data_[i]);
                }
            }

            ~UniformBuffer()
            {
                for (uint32_t i = 0; i < frame_count_; ++i)
                {
                    if (mapped_data_[i])
                    {
                        vkUnmapMemory(device_->device(), memories_[i]);
                    }
                    if (buffers_[i] != VK_NULL_HANDLE)
                    {
                        vkDestroyBuffer(device_->device(), buffers_[i], nullptr);
                    }
                    if (memories_[i] != VK_NULL_HANDLE)
                    {
                        vkFreeMemory(device_->device(), memories_[i], nullptr);
                    }
                }
            }

            // Non-copyable
            UniformBuffer(const UniformBuffer&)            = delete;
            UniformBuffer& operator=(const UniformBuffer&) = delete;

            // Update data for current frame
            void update(const T& data)
            {
                std::memcpy(mapped_data_[current_frame_], &data, sizeof(T));
            }

            // Update data for specific frame
            void update(uint32_t frame_index, const T& data)
            {
                std::memcpy(mapped_data_[frame_index], &data, sizeof(T));
            }

            // Set current frame
            void set_frame(uint32_t frame)
            {
                current_frame_ = frame % frame_count_;
            }

            // Get buffer for current frame
            VkBuffer current_buffer() const { return buffers_[current_frame_]; }

            // Get buffer for specific frame
            VkBuffer buffer(uint32_t frame) const { return buffers_[frame % frame_count_]; }

            uint32_t frame_count() const { return frame_count_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            std::vector<VkBuffer>          buffers_;
            std::vector<VkDeviceMemory>    memories_;
            std::vector<void*>             mapped_data_;
            uint32_t                       frame_count_;
            uint32_t                       current_frame_;
    };
} // namespace vulkan_engine::vulkan