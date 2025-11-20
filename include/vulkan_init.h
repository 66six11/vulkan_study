#pragma once

#include "Platform.h"
#include "constants.h"
#include <vector>
#include <string>

/**
 * @brief 创建Vulkan实例
 * 
 * 创建Vulkan实例，这是使用Vulkan API的第一步，用于初始化Vulkan库并设置全局状态
 * 
 * @param instance [out] 创建的Vulkan实例
 * @param window 指向GLFW窗口的指针，用于获取必要的扩展
 */
void createInstance(VkInstance& instance, GLFWwindow* window);

/**
 * @brief 设置调试信息回调
 * 
 * 配置Vulkan调试信息回调函数，用于捕获验证层的警告和错误信息
 * 
 * @param instance Vulkan实例
 */
void setupDebugMessenger(VkInstance instance);

/**
 * @brief 创建窗口表面
 * 
 * 创建连接Vulkan和本地窗口系统的表面对象
 * 
 * @param instance Vulkan实例
 * @param window 指向GLFW窗口的指针
 * @param surface [out] 创建的表面对象
 */
void createSurface(VkInstance instance, GLFWwindow* window, VkSurfaceKHR& surface);

/**
 * @brief 选择合适的物理设备
 * 
 * 枚举系统中的物理设备并选择一个支持所需功能的设备
 * 
 * @param instance Vulkan实例
 * @param surface 窗口表面，用于检查设备对表面的支持
 * @param physicalDevice [out] 选中的物理设备
 */
void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice& physicalDevice);

/**
 * @brief 创建逻辑设备
 * 
 * 基于物理设备创建逻辑设备，逻辑设备是与GPU交互的主要接口
 * 
 * @param physicalDevice 物理设备
 * @param surface 窗口表面，用于检查呈现队列的支持
 * @param device [out] 创建的逻辑设备
 * @param indices 队列族索引
 * @param graphicsQueue [out] 图形队列
 * @param presentQueue [out] 呈现队列
 */
void createLogicalDevice(VkPhysicalDevice   physicalDevice,
                         VkSurfaceKHR       surface,
                         VkDevice&          device,
                         QueueFamilyIndices indices,
                         VkQueue&           graphicsQueue,
                         VkQueue&           presentQueue);
