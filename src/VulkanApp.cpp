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
    // instance + debug
    createInstance(rc.instance, window);
    setupDebugMessenger(rc.instance);

    // surface
    createSurface(rc.instance, window, rc.surface);

    // 设备
    VulkanDeviceConfig cfg{};
    cfg.requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    rc.device              = std::make_unique<VulkanDevice>(rc.instance, rc.surface, cfg);

    // 资源/描述符管理器
    rc.resources   = std::make_unique<ResourceManager>(*rc.device);
    rc.descriptors = std::make_unique<DescriptorSetManager>(*rc.device);

    // 主命令池和同步对象
    QueueFamilyIndices indices = findQueueFamilies(rc.device->physicalDevice(), rc.surface);
    createCommandPool(rc.device->device(), indices, rc.mainCommandPool);
    createSemaphores(rc.device->device(), rc.imageAvailable, rc.renderFinished);

    // 初次创建 swapchain
    createOrRecreateSwapchain(rc);
}

void Application::createOrRecreateSwapchain(RenderContext& rc)
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


    vkDeviceWaitIdle(rc.device->device());

    rc.swapchain.destroy();
    new(&rc.swapchain) SwapchainResources(rc.device->device(), rc.mainCommandPool);

    QueueFamilyIndices indices = findQueueFamilies(rc.device->physicalDevice(), rc.surface);

    createSwapChain(rc.device->physicalDevice(),
                    rc.device->device(),
                    rc.surface,
                    indices,
                    rc.swapchain.swapchain,
                    rc.swapchain.images,
                    rc.swapchain.imageFormat,
                    rc.swapchain.extent);

    createImageViews(rc.device->device(), rc.swapchain.images, rc.swapchain.imageFormat, rc.swapchain.imageViews);

    createRenderPass(rc.device->device(), rc.swapchain.imageFormat, rc.swapchain.renderPass);

    createGraphicsPipeline(rc.device->device(),
                           rc.swapchain.extent,
                           rc.swapchain.renderPass,
                           rc.swapchain.pipelineLayout,
                           rc.swapchain.graphicsPipeline);

    createFramebuffers(rc.device->device(),
                       rc.swapchain.imageViews,
                       rc.swapchain.renderPass,
                       rc.swapchain.extent,
                       rc.swapchain.framebuffers);

    createCommandBuffers(rc.device->device(),
                         rc.mainCommandPool,
                         rc.swapchain.framebuffers,
                         rc.swapchain.renderPass,
                         rc.swapchain.extent,
                         rc.swapchain.graphicsPipeline,
                         rc.swapchain.imageViews,
                         rc.swapchain.commandBuffers);

    for (size_t i = 0; i < rc.swapchain.commandBuffers.size(); ++i)
    {
        recordCommandBuffer(rc.swapchain.commandBuffers[i],
                            static_cast<uint32_t>(i),
                            rc.swapchain.renderPass,
                            rc.swapchain.extent,
                            rc.swapchain.graphicsPipeline,
                            rc.swapchain.framebuffers[i]);
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
            createOrRecreateSwapchain(rc); // 或者内部实现调用同一套逻辑的 recreateSwapChain()
            continue;
        }

        drawFrame(rc.device->device(),
                  rc.swapchain.swapchain,
                  rc.device->graphicsQueue().handle,
                  rc.device->presentQueue().handle,
                  rc.swapchain.commandBuffers,
                  rc.imageAvailable,
                  rc.renderFinished);
    }

    vkDeviceWaitIdle(rc.device->device());
}

/**
 * @brief 清理资源
 * 
 * 按照创建的相反顺序销毁所有Vulkan对象，释放资源，防止内存泄漏
 * 这是Vulkan应用程序生命周期管理的重要部分
 */
void Application::cleanup()
{
    vkDeviceWaitIdle(rc.device->device());

    rc.swapchain.destroy(); // 内部使用 rc.device->device() free cmd buffers + destroy pipeline/fb/...

    vkDestroySemaphore(rc.device->device(), rc.renderFinished, nullptr);
    vkDestroySemaphore(rc.device->device(), rc.imageAvailable, nullptr);
    vkDestroyCommandPool(rc.device->device(), rc.mainCommandPool, nullptr);

    rc.descriptors.reset();
    rc.resources.reset();

    VkDevice raw = rc.device->device();
    rc.device.reset(); // 不再 destroy VkDevice，只丢弃封装
    vkDestroyDevice(raw, nullptr);

    vkDestroySurfaceKHR(rc.instance, rc.surface, nullptr);
    vkDestroyInstance(rc.instance, nullptr);
}
