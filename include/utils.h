#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <vector>

/**
 * @brief 检查设备是否适合
 * 
 * 检查物理设备是否满足应用程序的需求，包括队列族支持、扩展支持和交换链支持
 * 
 * @param device 物理设备
 * @param surface 窗口表面
 * @return 如果设备适合则返回true，否则返回false
 */
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief 查找队列族
 * 
 * 查找物理设备中支持图形和呈现操作的队列族
 * 
 * @param device 物理设备
 * @param surface 窗口表面
 * @return 包含图形和呈现队列族索引的结构体
 */
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief 检查设备扩展支持
 * 
 * 检查物理设备是否支持所需的设备扩展
 * 
 * @param device 物理设备
 * @return 如果所有必需的扩展都支持则返回true，否则返回false
 */
bool checkDeviceExtensionSupport(VkPhysicalDevice device);

/**
 * @brief 查询交换链支持详情
 * 
 * 查询物理设备对指定表面的交换链支持详情，包括能力、格式和呈现模式
 * 
 * @param device 物理设备
 * @param surface 窗口表面
 * @return 交换链支持详情结构体
 */
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief 选择交换链表面格式
 * 
 * 从可用的表面格式中选择最合适的格式
 * 
 * @param availableFormats 可用的表面格式集合
 * @return 选中的表面格式
 */
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

/**
 * @brief 选择交换链呈现模式
 * 
 * 从可用的呈现模式中选择最合适的模式
 * 
 * @param availablePresentModes 可用的呈现模式集合
 * @return 选中的呈现模式
 */
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

/**
 * @brief 选择交换链图像尺寸
 * 
 * 根据表面能力和窗口尺寸选择合适的交换链图像尺寸
 * 
 * @param capabilities 表面能力
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 选中的图像尺寸
 */
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);