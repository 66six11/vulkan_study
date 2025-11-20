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
    // 先选择物理设备
    pickPhysicalDevice(instance, surface, physicalDevice);
    // 再基于选定的物理设备查询队列族
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    createLogicalDevice(physicalDevice, surface, device, indices, graphicsQueue, presentQueue);

    // 3. Swapchain（交换链） & image view
    createSwapChain(physicalDevice,
                    device,
                    surface,
                    indices,
                    swapChain,
                    swapChainImages,
                    swapChainImageFormat,
                    swapChainExtent);
    createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);

    // 4. Render pass & pipeline & framebuffer
    createRenderPass(device, swapChainImageFormat, renderPass);
    createGraphicsPipeline(device, swapChainExtent, renderPass, pipelineLayout, graphicsPipeline);
    createFramebuffers(device, swapChainImageViews, renderPass, swapChainExtent, swapChainFramebuffers);

    // 5. Command pool/buffers & sync
    createCommandPool(device, indices, commandPool);
    createCommandBuffers(device,
                         commandPool,
                         swapChainFramebuffers,
                         renderPass,
                         swapChainExtent,
                         graphicsPipeline,
                         swapChainImageViews,
                         commandBuffers);
    createSemaphores(device, imageAvailableSemaphore, renderFinishedSemaphore);

    // 6. 录制命令缓冲（如果你以后要支持窗口 resize，这一部分可以提取出来重用）
    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        recordCommandBuffer(commandBuffers[i],
                            static_cast<uint32_t>(i),
                            renderPass,
                            swapChainExtent,
                            graphicsPipeline,
                            swapChainFramebuffers[i]);
    }
}

void Application::recreateSwapChain()
{
    // 1. 处理窗口被最小化为 0x0 的情况：此时不能创建 swapchain，等待恢复
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // 2. 等待设备空闲，确保没有命令在使用旧的 swapchain 资源
    vkDeviceWaitIdle(device);

    // 3. 按依赖关系的逆序销毁旧的与 swapchain 相关的资源
    // 3.1 销毁帧缓冲
    for (auto framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapChainFramebuffers.clear();

    // 3.2 销毁图形管线和管线布局
    if (graphicsPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    // 3.3 销毁渲染通道
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    // 3.4 销毁图像视图
    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainImageViews.clear();

    // 3.5 销毁旧的交换链
    if (swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }

    // 4. 重新创建与 swapchain 相关的所有资源
    //    顺序基本与 initVulkan 中的 3~5 步一致

    // 4.1 我们仍然使用之前挑选好的 physicalDevice / surface / 队列族
    //     注意：这里需要重新获取队列族索引
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    // 4.2 创建新的交换链和交换链图像
    createSwapChain(physicalDevice,
                    device,
                    surface,
                    indices,
                    swapChain,
                    swapChainImages,
                    swapChainImageFormat,
                    swapChainExtent);

    // 4.3 为新的交换链图像创建图像视图
    createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);

    // 4.4 创建新的渲染通道
    createRenderPass(device, swapChainImageFormat, renderPass);

    // 4.5 基于新的 extent 和 render pass 创建图形管线
    createGraphicsPipeline(device, swapChainExtent, renderPass, pipelineLayout, graphicsPipeline);

    // 4.6 为每个图像视图创建新的帧缓冲
    createFramebuffers(device, swapChainImageViews, renderPass, swapChainExtent, swapChainFramebuffers);

    // 4.7 重新分配 / 录制命令缓冲
    //     这里沿用原来的 commandPool，直接重新创建一批命令缓冲并录制
    createCommandBuffers(device,
                         commandPool,
                         swapChainFramebuffers,
                         renderPass,
                         swapChainExtent,
                         graphicsPipeline,
                         swapChainImageViews,
                         commandBuffers);

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        recordCommandBuffer(commandBuffers[i],
                            static_cast<uint32_t>(i),
                            renderPass,
                            swapChainExtent,
                            graphicsPipeline,
                            swapChainFramebuffers[i]);
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
            recreateSwapChain();
            continue; // 本轮循环不再 drawFrame，避免对已销毁资源提交命令
        }
     
        drawFrame(device,
                  swapChain,
                  graphicsQueue,
                  presentQueue,
                  commandBuffers,
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
    // 清理同步对象
    // 销毁渲染完成信号量
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    // 销毁图像可用信号量
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    // 清理命令池（这会自动释放所有从该池分配的命令缓冲）
    vkDestroyCommandPool(device, commandPool, nullptr);

    // 清理帧缓冲
    // 遍历并销毁所有帧缓冲对象
    for (auto framebuffer : swapChainFramebuffers)
    {
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
    for (auto imageView : swapChainImageViews)
    {
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


