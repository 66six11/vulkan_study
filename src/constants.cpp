#include "../include/constants.h"

#include <vulkan/vulkan_core.h>

/**
 * @brief 窗口宽度常量
 * 
 * 定义应用程序窗口的宽度（像素），用于初始化GLFW窗口和交换链尺寸选择
 */
const uint32_t WIDTH = 800;

/**
 * @brief 窗口高度常量
 * 
 * 定义应用程序窗口的高度（像素），用于初始化GLFW窗口和交换链尺寸选择
 */
const uint32_t HEIGHT = 600;

/**
 * @brief 设备扩展列表
 * 
 * 定义应用程序需要启用的设备扩展列表，当前仅包含交换链扩展，
 * 这是实现窗口呈现所必需的扩展
 */
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};