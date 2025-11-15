#pragma once

#include <optional>
#include <vector>

// 声明窗口常量
extern const uint32_t WIDTH;   // 窗口宽度
extern const uint32_t HEIGHT;  // 窗口高度

// 定义需要的设备扩展
extern const std::vector<const char*> deviceExtensions;

// 定义交换链支持的细节结构
// 封装物理设备对交换链的支持信息，包括能力、格式和呈现模式
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;  // 表面能力，包括最小/最大图像数量、尺寸范围等
    std::vector<VkSurfaceFormatKHR> formats;  // 支持的表面格式
    std::vector<VkPresentModeKHR> presentModes;  // 支持的呈现模式
};

// 定义队列族索引结构
// 用于存储图形队列和呈现队列的索引，便于后续创建逻辑设备
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;  // 图形队列族索引，用于图形命令提交
    std::optional<uint32_t> presentFamily;   // 呈现队列族索引，用于将图像呈现到屏幕

    // 检查是否找到了所有必需的队列族
    bool isComplete() const {
        // 如果两个队列族都有值，则返回true
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};