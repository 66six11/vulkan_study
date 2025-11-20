// Application.h
#pragma once // 防止头文件被多次包含 - 避免重复包含导致的重定义错误

// 包含Vulkan头文件 - 使用GLFW的宏定义来自动包含Vulkan.h
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "Platform.h"
#include <vector>
#include <optional>
#include <set>
#include <string>
#include "constants.h"

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

        struct SwapchainResources
        {
            VkSwapchainKHR               swapchain = VK_NULL_HANDLE;
            std::vector<VkImage>         images;
            VkFormat                     imageFormat = VK_FORMAT_UNDEFINED;
            VkExtent2D                   extent      = {0, 0};
            std::vector<VkImageView>     imageViews;
            VkRenderPass                 renderPass       = VK_NULL_HANDLE;
            VkPipelineLayout             pipelineLayout   = VK_NULL_HANDLE;
            VkPipeline                   graphicsPipeline = VK_NULL_HANDLE;
            std::vector<VkFramebuffer>   framebuffers;
            std::vector<VkCommandBuffer> commandBuffers;

            // 非拥有指针/句柄：由外部提供（device, commandPool 等），这里不负责销毁
            VkDevice      device      = VK_NULL_HANDLE;
            VkCommandPool commandPool = VK_NULL_HANDLE;

            // 析构函数只在 device 非空时销毁 Vulkan 资源
            ~SwapchainResources()
            {
                if (device == VK_NULL_HANDLE)
                    return;

                for (auto framebuffer : framebuffers)
                    vkDestroyFramebuffer(device, framebuffer, nullptr);

                if (graphicsPipeline != VK_NULL_HANDLE)
                    vkDestroyPipeline(device, graphicsPipeline, nullptr);

                if (pipelineLayout != VK_NULL_HANDLE)
                    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

                if (renderPass != VK_NULL_HANDLE)
                    vkDestroyRenderPass(device, renderPass, nullptr);

                for (auto view : imageViews)
                    vkDestroyImageView(device, view, nullptr);

                if (swapchain != VK_NULL_HANDLE)
                    vkDestroySwapchainKHR(device, swapchain, nullptr);
            }

            // 禁止拷贝，允许移动（真实项目中通常这么做）
            SwapchainResources()                                     = default;
            SwapchainResources(const SwapchainResources&)            = delete;
            SwapchainResources& operator=(const SwapchainResources&) = delete;

            SwapchainResources(SwapchainResources&& other) noexcept { *this = std::move(other); }

            SwapchainResources& operator=(SwapchainResources&& other) noexcept
            {
                if (this != &other)
                {
                    this->~SwapchainResources();
                    std::memcpy(this, &other, sizeof(SwapchainResources));
                    // 把 other 置为默认状态，避免 double free
                    other.device           = VK_NULL_HANDLE;
                    other.swapchain        = VK_NULL_HANDLE;
                    other.renderPass       = VK_NULL_HANDLE;
                    other.pipelineLayout   = VK_NULL_HANDLE;
                    other.graphicsPipeline = VK_NULL_HANDLE;
                }
                return *this;
            }
        };

    private:
        // 窗口和Vulkan实例相关成员变量
        GLFWwindow*  window   = nullptr;        // GLFW窗口对象，用于创建和管理应用程序窗口
        VkInstance   instance = VK_NULL_HANDLE; // Vulkan实例，是与Vulkan驱动程序交互的入口点
        VkSurfaceKHR surface  = VK_NULL_HANDLE; // 窗口表面，用于连接窗口系统和Vulkan，实现图像呈现

        // 物理和逻辑设备相关成员变量
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // 物理设备（GPU），代表系统中的实际图形硬件
        VkDevice         device         = VK_NULL_HANDLE; // 逻辑设备，用于与GPU进行交互，是应用程序与物理设备通信的主要接口


        SwapchainResources swapchainResources;
        VkCommandPool                commandPool = VK_NULL_HANDLE; // 命令池，用于分配命令缓冲，管理命令缓冲的内存
        // 同步相关成员变量
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE; // 图形-呈现同步信号量，用于同步图像获取和渲染开始
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE; // 呈现-图形同步信号量，用于同步渲染完成和图像呈现

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
        void createOrRecreateSwapchain();


        /**
        * @brief 重建交换链及相关资源
        *
        * 当窗口大小变化或交换链失效时，销毁并重新创建所有依赖交换链的资源。
        */
        void recreateSwapChain();

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
