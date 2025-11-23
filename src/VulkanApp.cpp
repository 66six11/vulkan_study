// HelloTriangleApplication.cpp
// 定义GLFW包含Vulkan头文件的宏，这样GLFW会自动包含Vulkan头文件

#include <chrono>
#include <stdexcept>
#include "Application.h"
#include "constants.h"

#include "VulkanRenderer.h"

/**
 * @brief 运行应用程序的主要函数
 * 
 * 按顺序执行初始化、主循环和清理操作，是应用程序的主控制流程
 */
void Application::run()
{
    initWindow();

    // 创建后端 Renderer（此处直接 new VulkanRenderer，之后可以做工厂）
    renderer_ = std::make_unique<VulkanRenderer>();
    renderer_->initialize(window, WIDTH, HEIGHT);

    mainLoop();

    // Renderer 持有的 Vulkan 资源在析构时清理
    renderer_->waitIdle();
    renderer_.reset();

    cleanup(); // 这里只清理窗口和与 Vulkan 无关的资源
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
 * @brief 主循环
 * 
 * 持续处理窗口事件并渲染帧，直到窗口关闭，这是应用程序的渲染循环核心
 */
void Application::mainLoop()
{
    // Application 成员变量（或 main 里的静态变量）：
    using Clock                  = std::chrono::high_resolution_clock;
    Clock::time_point lastTime   = Clock::now();
    uint64_t          frameIndex = 0;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        // 处理最小化窗口：宽高为 0 时，不要操作 swapchain
        if (width == 0 || height == 0)
        {
            glfwWaitEvents();
            continue;
        }

        // 只要窗口大小变了，或者 renderer 发现 swapchain 过期，就重建一次
        if (framebufferResized)
        {
            framebufferResized = false;
            renderer_->resize(width, height);
            continue;
        }

        auto                         now   = Clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime                           = now;


        FrameContext ctx{};
        ctx.timing.deltaTime  = delta.count(); // 单位：秒，例如 0.016f ≈ 60FPS
        ctx.timing.frameIndex = frameIndex++;  // 从 0 开始递增


        // 注意：现在 beginFrame 返回 bool
        if (!renderer_->beginFrame(ctx))
        {
            // 这里通常是 acquire 返回 OUT_OF_DATE，下一轮 loop 会走到上面的 resize 分支
            continue;
        }

        renderer_->renderFrame();
    }

    renderer_->waitIdle();
}

/**
 * @brief 清理资源
 * 
 * 按照创建的相反顺序销毁所有Vulkan对象，释放资源，防止内存泄漏
 * 这是Vulkan应用程序生命周期管理的重要部分
 */
void Application::cleanup()
{
    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}
