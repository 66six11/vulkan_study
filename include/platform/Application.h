// Application.h
#pragma once // 防止头文件被多次包含 - 避免重复包含导致的重定义错误

// 包含Vulkan头文件 - 使用GLFW的宏定义来自动包含Vulkan.h


#include <memory>      // 加这一行
#include "platform/Platform.h"
#include "renderer/Renderer.h"


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

        MeshHandle sphereMesh_{};
        Renderable sphereRenderable_{};
        bool       sphereInitialized_ = false;

    private:
        // 窗口和Vulkan实例相关成员变量
        GLFWwindow*               window = nullptr; // GLFW窗口对象，用于创建和管理应用程序窗口
        std::unique_ptr<Renderer> renderer_;
        uint64_t                  frameIndex_ = 0;


        // 应用程序主要函数
        /**
         * @brief 初始化GLFW窗口
         * 
         * 创建和配置GLFW窗口，设置窗口属性，为后续Vulkan表面创建做准备
         */
        void initWindow();

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
