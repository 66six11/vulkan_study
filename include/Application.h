// Application.h
#pragma once // 防止头文件被多次包含 - 避免重复包含导致的重定义错误

// 包含Vulkan头文件 - 使用GLFW的宏定义来自动包含Vulkan.h
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <optional>
#include <set>
#include <string>

#include "constants.h"

class Application {
public:
    // 运行应用程序的主要函数
    void run();

private:
    // 窗口和Vulkan实例相关成员变量
    GLFWwindow *window = nullptr;  // GLFW窗口对象
    VkInstance instance = VK_NULL_HANDLE;  // Vulkan实例，用于与Vulkan驱动程序交互
    VkSurfaceKHR surface = VK_NULL_HANDLE;  // 窗口表面，用于连接窗口系统和Vulkan

    // 物理和逻辑设备相关成员变量
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;  // 物理设备（GPU）
    VkDevice device = VK_NULL_HANDLE;  // 逻辑设备，用于与GPU进行交互

    // 队列相关成员变量
    VkQueue graphicsQueue = VK_NULL_HANDLE;  // 图形队列，用于提交图形命令
    VkQueue presentQueue = VK_NULL_HANDLE;   // 呈现队列，用于将图像呈现到屏幕

    // 交换链相关成员变量
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;  // 交换链，用于管理呈现图像
    std::vector<VkImage> swapChainImages;       // 交换链中的图像
    VkFormat swapChainImageFormat;              // 交换链图像格式
    VkExtent2D swapChainExtent;                 // 交换链图像尺寸
    std::vector<VkImageView> swapChainImageViews;  // 图像视图，用于访问图像数据

    // 渲染通道相关成员变量
    VkRenderPass renderPass = VK_NULL_HANDLE;  // 渲染通道，定义渲染操作的附件和子通道

    // 图形管线相关成员变量
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;  // 管线布局，定义着色器使用的资源布局
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;      // 图形管线，定义图形渲染的完整状态

    // 帧缓冲相关成员变量
    std::vector<VkFramebuffer> swapChainFramebuffers;  // 帧缓冲，用于存储渲染附件

    // 命令相关成员变量
    VkCommandPool commandPool = VK_NULL_HANDLE;        // 命令池，用于分配命令缓冲
    std::vector<VkCommandBuffer> commandBuffers;       // 命令缓冲，用于记录命令

    // 同步相关成员变量
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;  // 图像可用信号量
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;  // 渲染完成信号量

    // 应用程序主要函数

    void initWindow();       // 初始化GLFW窗口

    void initVulkan();       // 初始化Vulkan

    void mainLoop();         // 主循环

    void cleanup();          // 清理资源
};


