// HelloTriangleApplication.cpp
// 定义GLFW包含Vulkan头文件的宏，这样GLFW会自动包含Vulkan头文件

#include "Application.h"
#include "vulkan_init.h"
#include "swapchain_management.h"
#include "rendering.h"
#include "command_buffer_sync.h"
#include "utils.h"
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
#include "constants.h"

// 引入常量定义
#include "constants.h"

/**
 * @brief 运行应用程序的主要函数
 * 
 * 按顺序执行初始化、主循环和清理操作，是应用程序的主控制流程
 */
void Application::run()
{
    // 初始化GLFW窗口
    initWindow();
    // 初始化Vulkan相关对象
    initVulkan();
    // 进入主循环，持续渲染直到窗口关闭
    mainLoop();
    // 清理所有分配的Vulkan资源
    cleanup();
}

// GLFW framebuffer 大小变化回调：仅设置一个标志，真正的重建放到主循环里做
void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->framebufferResized = true;
    }
}

/**
 * @brief 初始化GLFW窗口
 * 
 * 初始化GLFW库并创建应用程序窗口，设置窗口属性
 */
void Application::initWindow()
{
    glfwInit();
    if (!glfwInit())
    {
        throw std::runtime_error("failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", nullptr, nullptr);

    // 为后续支持窗口大小变化做准备：记录 this 指针，方便回调中访问 Application 实例
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("failed to create GLFW window");
    }
}

/**
 * @brief 初始化Vulkan
 * 
 * 初始化所有Vulkan相关对象，包括实例、表面、物理设备、逻辑设备、
 * 交换链、渲染通道、图形管线、帧缓冲、命令池和同步对象
 */

void Application::initVulkan()
{
    // 1. Vulkan 实例 & 调试
    createInstance(instance, window);
    setupDebugMessenger(instance);

    // 2. Surface & 物理/逻辑设备
    createSurface(instance, window, surface);
    pickPhysicalDevice(instance, surface, physicalDevice);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    createLogicalDevice(physicalDevice,
                        surface,
                        device,
                        indices,
                        graphicsQueue,
                        presentQueue);

    // 3. 只创建一次的、与窗口大小无关的资源：命令池 + 信号量
    createCommandPool(device, indices, commandPool);
    createSemaphores(device, imageAvailableSemaphore, renderFinishedSemaphore);

    // 4. 第一次创建整套与窗口大小相关的 swapchain 资源
    createOrRecreateSwapchain();
}

void Application::createOrRecreateSwapchain()
{
    // 1. 处理 0x0（最小化）窗口
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    // 2. 先销毁旧的（通过 RAII 完成）
    swapchainResources.~SwapchainResources();
    new(&swapchainResources) SwapchainResources();

    // 3. 填写基础字段（非拥有句柄）
    swapchainResources.device      = device;
    swapchainResources.commandPool = commandPool;

    // 4. 像原来的 initVulkan 一样创建整条链
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    // 创建交换链及相关资源
    createSwapChain(physicalDevice,
                    device,
                    surface,
                    indices,
                    swapchainResources.swapchain,
                    swapchainResources.images,
                    swapchainResources.imageFormat,
                    swapchainResources.extent);
    // 创建图像视图
    createImageViews(device,
                     swapchainResources.images,
                     swapchainResources.imageFormat,
                     swapchainResources.imageViews);
    // 创建渲染通道
    createRenderPass(device,
                     swapchainResources.imageFormat,
                     swapchainResources.renderPass);
    // 创建图形管线
    createGraphicsPipeline(device,
                           swapchainResources.extent,
                           swapchainResources.renderPass,
                           swapchainResources.pipelineLayout,
                           swapchainResources.graphicsPipeline);
    // 创建帧缓冲
    createFramebuffers(device,
                       swapchainResources.imageViews,
                       swapchainResources.renderPass,
                       swapchainResources.extent,
                       swapchainResources.framebuffers);
    // 创建命令缓冲
    createCommandBuffers(device,
                         commandPool,
                         swapchainResources.framebuffers,
                         swapchainResources.renderPass,
                         swapchainResources.extent,
                         swapchainResources.graphicsPipeline,
                         swapchainResources.imageViews,
                         swapchainResources.commandBuffers);
    // 记录命令缓冲
    for (size_t i = 0; i < swapchainResources.commandBuffers.size(); ++i)
    {
        recordCommandBuffer(swapchainResources.commandBuffers[i],
                            static_cast<uint32_t>(i),
                            swapchainResources.renderPass,
                            swapchainResources.extent,
                            swapchainResources.graphicsPipeline,
                            swapchainResources.framebuffers[i]);
    }
}


/**
 * @brief 主循环
 * 
 * 持续处理窗口事件并渲染帧，直到窗口关闭，这是应用程序的渲染循环核心
 */
void Application::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (framebufferResized)
        {
            framebufferResized = false;
            createOrRecreateSwapchain(); // 或者内部实现调用同一套逻辑的 recreateSwapChain()
            continue;
        }

        drawFrame(device,
                  swapchainResources.swapchain,
                  graphicsQueue,
                  presentQueue,
                  swapchainResources.commandBuffers,
                  imageAvailableSemaphore,
                  renderFinishedSemaphore);
    }

    vkDeviceWaitIdle(device);
}

/**
 * @brief 清理资源
 * 
 * 按照创建的相反顺序销毁所有Vulkan对象，释放资源，防止内存泄漏
 * 这是Vulkan应用程序生命周期管理的重要部分
 */
void Application::cleanup()
{
    // 1. 清理 swapchain 相关资源（如果用 SwapchainResources，这里只需要：）
    swapchainResources.destroy(); // 或依靠它的析构，不要两边都 destroy

    // 2. 同步对象和命令池
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);

    // 3. 最后销毁 device / surface / instance
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}
