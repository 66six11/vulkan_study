// HelloTriangleApplication.cpp
// 定义GLFW包含Vulkan头文件的宏，这样GLFW会自动包含Vulkan头文件
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../include/Application.h"
#include "../include/vulkan_init.h"
#include "../include/swapchain_management.h"
#include "../include/rendering.h"
#include "../include/command_buffer_sync.h"
#include "../include/utils.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <vulkan/vulkan_core.h>

// 引入常量定义
#include "../include/constants.h"

/**
 * @brief 运行应用程序的主要函数
 * 
 * 按顺序执行初始化、主循环和清理操作，是应用程序的主控制流程
 */
void Application::run() {
    // 初始化GLFW窗口
    initWindow();
    // 初始化Vulkan相关对象
    initVulkan();
    // 进入主循环，持续渲染直到窗口关闭
    mainLoop();
    // 清理所有分配的Vulkan资源
    cleanup();
}

/**
 * @brief 初始化GLFW窗口
 * 
 * 初始化GLFW库并创建应用程序窗口，设置窗口属性
 */
void Application::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
}

/**
 * @brief 初始化Vulkan
 * 
 * 初始化所有Vulkan相关对象，包括实例、表面、物理设备、逻辑设备、
 * 交换链、渲染通道、图形管线、帧缓冲、命令池和同步对象
 */
void Application::initVulkan() {
    createInstance(instance, window);
    setupDebugMessenger(instance);
    createSurface(instance, window, surface);
    pickPhysicalDevice(instance, surface, physicalDevice);
    createLogicalDevice(physicalDevice, surface, device, 
                       findQueueFamilies(physicalDevice, surface), 
                       graphicsQueue, presentQueue);
    createSwapChain(physicalDevice, device, surface,
                   findQueueFamilies(physicalDevice, surface),
                   swapChain, swapChainImages, swapChainImageFormat, swapChainExtent);
    createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
    createRenderPass(device, swapChainImageFormat, renderPass);
    createGraphicsPipeline(device, swapChainExtent, renderPass, pipelineLayout, graphicsPipeline);
    createFramebuffers(device, swapChainImageViews, renderPass, swapChainExtent, swapChainFramebuffers);
    createCommandPool(device, findQueueFamilies(physicalDevice, surface), commandPool);
    createCommandBuffers(device, commandPool, swapChainFramebuffers, renderPass, swapChainExtent,
                        graphicsPipeline, swapChainImageViews, commandBuffers);
    createSemaphores(device, imageAvailableSemaphore, renderFinishedSemaphore);
    
    // 记录命令缓冲
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        recordCommandBuffer(commandBuffers[i], static_cast<uint32_t>(i), 
                           renderPass, swapChainExtent, graphicsPipeline, 
                           swapChainFramebuffers[i]);
    }
}

/**
 * @brief 主循环
 * 
 * 持续处理窗口事件并渲染帧，直到窗口关闭，这是应用程序的渲染循环核心
 */
void Application::mainLoop() {
    // 循环直到窗口应该关闭
    while (!glfwWindowShouldClose(window)) {
        // 处理窗口事件（如键盘输入、鼠标移动等）
        glfwPollEvents();
        // 绘制一帧
        drawFrame(device, swapChain, graphicsQueue, presentQueue, commandBuffers,
                 imageAvailableSemaphore, renderFinishedSemaphore);
    }
}

/**
 * @brief 清理资源
 * 
 * 按照创建的相反顺序销毁所有Vulkan对象，释放资源，防止内存泄漏
 * 这是Vulkan应用程序生命周期管理的重要部分
 */
void Application::cleanup() {
    // 清理同步对象
    // 销毁渲染完成信号量
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    // 销毁图像可用信号量
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    // 清理命令池（这会自动释放所有从该池分配的命令缓冲）
    vkDestroyCommandPool(device, commandPool, nullptr);

    // 清理帧缓冲
    // 遍历并销毁所有帧缓冲对象
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // 清理管线相关对象
    // 销毁图形管线
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    // 销毁管线布局
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // 销毁渲染通道
    vkDestroyRenderPass(device, renderPass, nullptr);

    // 清理图像视图
    // 遍历并销毁所有图像视图
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // 清理交换链
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    // 清理逻辑设备
    vkDestroyDevice(device, nullptr);

    // 清理窗口表面
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // 清理实例
    vkDestroyInstance(instance, nullptr);

    // 清理GLFW相关资源
    glfwDestroyWindow(window);
    glfwTerminate();
}


