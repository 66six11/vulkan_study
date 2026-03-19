#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>

namespace vulkan_engine::vulkan
{
    /**
     * @brief Vulkan API 错误异常类
     */
    class VulkanError : public std::runtime_error
    {
        public:
            VulkanError(VkResult result, const std::string& message, const char* file = nullptr, int line = 0)
                : std::runtime_error(format_message(result, message, file, line))
                , result_(result)
            {
            }

            VkResult result() const noexcept { return result_; }

            static const char* result_to_string(VkResult result) noexcept
            {
                switch (result)
                {
                    case VK_SUCCESS: return "VK_SUCCESS";
                    case VK_NOT_READY: return "VK_NOT_READY";
                    case VK_TIMEOUT: return "VK_TIMEOUT";
                    case VK_EVENT_SET: return "VK_EVENT_SET";
                    case VK_EVENT_RESET: return "VK_EVENT_RESET";
                    case VK_INCOMPLETE: return "VK_INCOMPLETE";
                    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
                    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
                    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
                    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
                    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
                    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
                    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
                    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
                    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
                    case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
                    default: return "VK_UNKNOWN_ERROR";
                }
            }

        private:
            VkResult result_;

            static std::string format_message(VkResult result, const std::string& message, const char* file, int line)
            {
                std::string result_str = result_to_string(result);
                if (file && line > 0)
                {
                    return "[VulkanError] " + message + " (" + result_str + ") at " + file + ":" + std::to_string(line);
                }
                return "[VulkanError] " + message + " (" + result_str + ")";
            }
    };
} // namespace vulkan_engine::vulkan

/**
 * @brief 检查 Vulkan API 调用结果，失败时抛出 VulkanError 异常
 */
#define VK_CHECK(result) \
    do { \
        if ((result) != VK_SUCCESS) { \
            throw vulkan_engine::vulkan::VulkanError(result, "Vulkan API call failed", __FILE__, __LINE__); \
        } \
    } while(0)

/**
 * @brief 检查 Vulkan API 调用结果，失败时抛出带有自定义消息的 VulkanError 异常
 */
#define VK_CHECK_MSG(result, message) \
    do { \
        if ((result) != VK_SUCCESS) { \
            throw vulkan_engine::vulkan::VulkanError(result, message, __FILE__, __LINE__); \
        } \
    } while(0)