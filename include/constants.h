#pragma once
//该文件是全局的常量定义文件，包含了一些常用的常量定义，比如窗口宽度、高度、设备扩展列表、交换链支持详情结构体、队列族索引结构体等。
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
/**
 * @brief 窗口宽度常量
 * 
 * 定义应用程序窗口的宽度（像素）
 */
extern const uint32_t WIDTH;

/**
 * @brief 窗口高度常量
 * 
 * 定义应用程序窗口的高度（像素）
 */
extern const uint32_t HEIGHT;

/**
 * @brief 设备扩展列表
 * 
 * 定义应用程序需要启用的设备扩展列表
 */
extern const std::vector<const char*> deviceExtensions;

/**
 * @brief 交换链支持详情结构体
 * 
 * 封装物理设备对交换链的支持信息，包括能力、格式和呈现模式
 */
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities; // 表面能力，包括最小/最大图像数量、尺寸范围等
    std::vector<VkSurfaceFormatKHR> formats;      // 支持的表面格式列表
    std::vector<VkPresentModeKHR>   presentModes; // 支持的呈现模式列表
};

/**
 * @brief 队列族索引结构体
 * 
 * 用于存储图形队列和呈现队列的索引，便于后续创建逻辑设备
 */
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily; // 图形队列族索引，用于图形命令提交
    std::optional<uint32_t> presentFamily;  // 呈现队列族索引，用于将图像呈现到屏幕

    /**
     * @brief 检查队列族是否完整
     * 
     * 检查是否找到了所有必需的队列族（图形队列和呈现队列）
     * 
     * @return 如果两个队列族都有值则返回true，否则返回false
     */
    bool isComplete() const
    {
        // 如果两个队列族都有值，则返回true
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// 是否启用验证层（调试构建为 true，发布构建为 false）
extern const bool enableValidationLayers;

// 验证层列表
extern const std::vector<const char*> validationLayers;
