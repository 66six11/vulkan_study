// Application.h
#pragma once // 防止头文件被多次包含 - 避免重复包含导致的重定义错误

// 包含Vulkan头文件 - 使用GLFW的宏定义来自动包含Vulkan.h
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "Platform.h"
#include "SwapchainResources.h"
#include <vector>
#include <optional>
#include <set>
#include <string>
#include "constants.h"
#include "DescriptorSetManager.h"
#include "ResourceManager.h"
#include "VulkanDevice.h"

struct RenderContext
{
    VkInstance                            instance{};
    VkSurfaceKHR                          surface{};
    std::unique_ptr<VulkanDevice>         device;      // 封装 physical + logical + queues
    std::unique_ptr<ResourceManager>      resources;   // Buffer/Image/Sampler
    std::unique_ptr<DescriptorSetManager> descriptors; // Descriptor pools/sets

    VkCommandPool mainCommandPool{}; // 用于 frame command buffers
    VkSemaphore   imageAvailable{};
    VkSemaphore   renderFinished{};

    SwapchainResources swapchain; // 当前 swapchain 的所有资源
};

/**
 * @brief Vulkan应用程序主类
 * 
 * Application类是整个Vulkan应用程序的核心，负责管理所有Vulkan资源的生命周期，
 * 包括窗口、实例、设备、交换链、渲染管线、命令缓冲等。它实现了Vulkan应用程序
 * 的完整初始化、渲染循环和资源清理流程。
 */
class Application
{
    public:
        /**
         * @brief 运行应用程序的主要函数
         * 
         * 按顺序执行初始化、主循环和清理操作，是应用程序的入口点方法调用链：
         * 1. initWindow() - 初始化GLFW窗口
         * 2. initVulkan() - 初始化所有Vulkan相关对象
         * 3. mainLoop() - 进入主渲染循环
         * 4. cleanup() - 清理所有分配的资源
         */
        void run();

        bool framebufferResized = false;

    private:
        // 窗口和Vulkan实例相关成员变量
        GLFWwindow*   window = nullptr; // GLFW窗口对象，用于创建和管理应用程序窗口
        RenderContext rc     = {};      // 渲染上下文，封装Vulkan实例、设备和资源管理器等
        // 队列相关成员变量
        VkQueue graphicsQueue = VK_NULL_HANDLE; // 图形队列，用于提交图形命令（如绘制操作、内存传输等）
        VkQueue presentQueue  = VK_NULL_HANDLE; // 呈现队列，用于将渲染完成的图像呈现到屏幕


        // 应用程序主要函数
        /**
         * @brief 初始化GLFW窗口
         * 
         * 创建和配置GLFW窗口，设置窗口属性，为后续Vulkan表面创建做准备
         */
        void initWindow();

        /**
         * @brief 初始化Vulkan
         * 
         * 初始化所有Vulkan相关对象，包括实例、调试、表面、物理设备、逻辑设备、
         * 交换链、图像视图、渲染通道、图形管线、帧缓冲、命令池、命令缓冲和同步对象
         */
        void initVulkan();

        /**
        * @brief 创建或重新创建交换链及相关资源
        * 
        * 如果交换链不存在则创建它，否则销毁旧的交换链资源并重新
        */
        void createOrRecreateSwapchain(RenderContext&);


        /**
        * @brief 重建交换链及相关资源
        *
        * 当窗口大小变化或交换链失效时，销毁并重新创建所有依赖交换链的资源。
        */
        void recreateSwapchain(RenderContext&);

        /**
         * @brief 主循环
         * 
         * 持续处理窗口事件并渲染帧，直到窗口关闭。这是应用程序的渲染循环核心
         */
        void mainLoop();

        /**
         * @brief 清理资源
         * 
         * 按照创建的相反顺序销毁所有Vulkan对象，释放资源，防止内存泄漏
         */
        void cleanup();
};
