#pragma once

#include "HelloTriangleApplication.h"

// 辅助功能相关函数
bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device);
QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device);
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device);
SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device);
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);