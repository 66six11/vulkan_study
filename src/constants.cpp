#include "../include/constants.h"

#include <vulkan/vulkan_core.h>

// 定义窗口常量
const uint32_t WIDTH = 800;   // 窗口宽度
const uint32_t HEIGHT = 600;  // 窗口高度

// 定义需要的设备扩展
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};