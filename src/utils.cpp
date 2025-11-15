#include "../include/HelloTriangleApplication.h"
#include "../include/utils.h"
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <limits>

// 检查设备是否适合
// 验证设备是否支持所有必需的功能
bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {
    // 检查设备是否支持所需的功能
    QueueFamilyIndices indices = findQueueFamilies(device);              // 查找队列族
    bool extensionsSupported = checkDeviceExtensionSupport(device);      // 检查扩展支持
    bool swapChainAdequate = false;                                      // 检查交换链支持

    // 如果扩展支持，则检查交换链是否足够
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        // 确保交换链支持至少一种格式和一种呈现模式
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // 设备适合的条件：
    // 1. 找到了所有必需的队列族
    // 2. 支持所有必需的扩展
    // 3. 交换链支持足够
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

// 查找队列族
// 查找支持图形和呈现操作的队列族
QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;  // 初始化队列族索引结构

    // 获取队列族数量
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // 获取所有队列族属性
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    // 遍历所有队列族
    for (const auto& queueFamily : queueFamilies) {
        // 检查队列族是否支持图形操作
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // 检查队列族是否支持呈现操作
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        // 如果找到了所有必需的队列族，则退出循环
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// 检查设备扩展支持
// 验证设备是否支持所有必需的扩展
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // 获取设备扩展数量
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // 获取所有可用的设备扩展
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // 创建必需扩展的集合
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // 从必需扩展集合中移除所有可用的扩展
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // 如果所有必需的扩展都可用，则集合为空
    return requiredExtensions.empty();
}

// 查询交换链支持
// 获取物理设备对交换链的支持信息
SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;  // 初始化支持细节结构

    // 获取表面能力
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // 获取表面格式数量
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // 如果有支持的格式，则获取所有格式
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // 获取呈现模式数量
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    // 如果有支持的呈现模式，则获取所有呈现模式
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// 选择交换链表面格式
// 从可用的表面格式中选择最佳格式
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // 遍历所有可用格式，寻找首选格式
    for (const auto& availableFormat : availableFormats) {
        // 首选格式：B8G8R8A8_UNORM + sRGB非线性颜色空间
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    // 如果没有找到首选格式，则返回第一个可用格式
    return availableFormats[0];
}

// 选择交换链呈现模式
// 从可用的呈现模式中选择最佳模式
VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // 遍历所有可用的呈现模式，寻找首选模式
    for (const auto& availablePresentMode : availablePresentModes) {
        // 首选模式：邮箱模式（三重缓冲）
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    
    // 如果没有找到首选模式，则返回FIFO模式（垂直同步）
    return VK_PRESENT_MODE_FIFO_KHR;
}

// 选择交换链尺寸
// 根据表面能力和窗口尺寸选择交换链图像尺寸
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // 检查当前尺寸是否有效（如果width为最大值，则表示需要指定尺寸）
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        // 返回当前尺寸
        return capabilities.currentExtent;
    } else {
        // 创建实际尺寸（基于窗口尺寸）
        VkExtent2D actualExtent = {WIDTH, HEIGHT};

        // 确保实际尺寸在允许的范围内
        actualExtent.width = std::max(capabilities.minImageExtent.width, 
                                    std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, 
                                     std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}