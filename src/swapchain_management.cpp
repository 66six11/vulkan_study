#include "../include/HelloTriangleApplication.h"
#include "../include/swapchain_management.h"
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>

// 创建交换链
// 创建交换链用于在渲染目标和屏幕之间交换图像
void HelloTriangleApplication::createSwapChain() {
    // 查询交换链支持细节
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // 选择合适的交换链参数
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);  // 表面格式
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);   // 呈现模式
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);                   // 图像尺寸

    // 确定图像数量（至少需要最小图像数量+1以避免等待）
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // 确保不超过最大图像数量限制
    if (swapChainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // 创建交换链
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;  // 结构体类型标识
    createInfo.surface = surface;                                    // 窗口表面
    createInfo.minImageCount = imageCount;                           // 图像数量
    createInfo.imageFormat = surfaceFormat.format;                   // 图像格式
    createInfo.imageColorSpace = surfaceFormat.colorSpace;           // 颜色空间
    createInfo.imageExtent = extent;                                 // 图像尺寸
    createInfo.imageArrayLayers = 1;                                 // 图像层数（通常为1）
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;     // 图像用途（作为颜色附件）

    // 设置队列族信息
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // 如果图形队列族和呈现队列族不同，需要并发共享
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;      // 并发共享模式
        createInfo.queueFamilyIndexCount = 2;                          // 队列族数量
        createInfo.pQueueFamilyIndices = queueFamilyIndices;           // 队列族索引数组
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;       // 独占模式
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;  // 预变换
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;             // Alpha合成
    createInfo.presentMode = presentMode;                                      // 呈现模式
    createInfo.clipped = VK_TRUE;                                              // 裁剪（允许裁剪不可见像素）
    createInfo.oldSwapchain = VK_NULL_HANDLE;                                  // 旧交换链（用于重建）

    // 创建交换链
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // 获取交换链图像
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // 保存交换链图像格式和尺寸
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// 创建图像视图
// 为交换链中的每个图像创建图像视图，用于访问图像数据
void HelloTriangleApplication::createImageViews() {
    // 为每个交换链图像创建图像视图
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // 填充图像视图创建信息
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;    // 结构体类型标识
        createInfo.image = swapChainImages[i];                          // 指向图像
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                    // 视图类型（2D图像）
        createInfo.format = swapChainImageFormat;                       // 图像格式
        // 设置颜色通道映射（默认映射）
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // 设置子资源范围
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // 图像方面（颜色）
        createInfo.subresourceRange.baseMipLevel = 0;                        // 基础mip等级
        createInfo.subresourceRange.levelCount = 1;                          // mip等级数量
        createInfo.subresourceRange.baseArrayLayer = 0;                      // 基础数组层
        createInfo.subresourceRange.layerCount = 1;                          // 数组层数量

        // 创建图像视图
        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}