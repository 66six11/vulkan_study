#pragma once

#include "constants.h"
#include <vector>

/**
 * @brief 创建交换链
 * 
 * 创建交换链对象，用于管理呈现图像，实现双缓冲或三缓冲以避免画面撕裂
 * 
 * @param physicalDevice 物理设备
 * @param device 逻辑设备
 * @param surface 窗口表面
 * @param indices 队列族索引
 * @param swapChain [out] 创建的交换链对象
 * @param swapChainImages [out] 交换链中的图像集合
 * @param swapChainImageFormat [out] 交换链图像格式
 * @param swapChainExtent [out] 交换链图像尺寸
 */
void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
                    QueueFamilyIndices indices, VkSwapchainKHR& swapChain, 
                    std::vector<VkImage>& swapChainImages, VkFormat& swapChainImageFormat, 
                    VkExtent2D& swapChainExtent);

/**
 * @brief 创建图像视图
 * 
 * 为交换链中的每个图像创建对应的图像视图，图像视图是图像与着色器之间的接口
 * 
 * @param device 逻辑设备
 * @param swapChainImages 交换链中的图像集合
 * @param swapChainImageFormat 交换链图像格式
 * @param swapChainImageViews [out] 创建的图像视图集合
 */
void createImageViews(VkDevice device, const std::vector<VkImage>& swapChainImages, 
                     VkFormat swapChainImageFormat, std::vector<VkImageView>& swapChainImageViews);