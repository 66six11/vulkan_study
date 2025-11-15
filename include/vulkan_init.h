#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <vector>
#include <string>

// Vulkan 初始化函数声明
void createInstance(VkInstance& instance, GLFWwindow* window);
void setupDebugMessenger(VkInstance instance);
void createSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR& surface);
void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice& physicalDevice);
void createLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice& device, 
                        QueueFamilyIndices indices, VkQueue& graphicsQueue, VkQueue& presentQueue);