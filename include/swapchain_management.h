#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <vector>

// 交换链管理函数声明
void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
                    QueueFamilyIndices indices, VkSwapchainKHR& swapChain, 
                    std::vector<VkImage>& swapChainImages, VkFormat& swapChainImageFormat, 
                    VkExtent2D& swapChainExtent);
void createImageViews(VkDevice device, const std::vector<VkImage>& swapChainImages, 
                     VkFormat swapChainImageFormat, std::vector<VkImageView>& swapChainImageViews);