// HelloTriangleApplication.cpp
// 定义GLFW包含Vulkan头文件的宏，这样GLFW会自动包含Vulkan头文件

#include <chrono>
#include <stdexcept>
#include <renderer/Vertex.h>

#include "core/constants.h"
#include "platform/Application.h"

#include "vulkan_backend/VulkanRenderer.h"

namespace
{
    struct MeshData
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
    };

    MeshData generateSphereMesh(float radius, uint32_t stacks, uint32_t slices)
    {
        MeshData data{};

        stacks = std::max(stacks, 2u);
        slices = std::max(slices, 3u);

        data.vertices.reserve((stacks + 1) * (slices + 1));
        data.indices.reserve(stacks * slices * 6);

        constexpr float PI     = 3.14159265358979323846f;
        constexpr float TWO_PI = 2.0f * PI;

        for (uint32_t i = 0; i <= stacks; ++i)
        {
            float v   = static_cast<float>(i) / static_cast<float>(stacks);
            float phi = v * PI; // [0, PI]

            float y   = std::cos(phi);
            float rXZ = std::sin(phi);

            for (uint32_t j = 0; j <= slices; ++j)
            {
                float u     = static_cast<float>(j) / static_cast<float>(slices);
                float theta = u * TWO_PI; // [0, 2PI]

                float x = rXZ * std::cos(theta);
                float z = rXZ * std::sin(theta);

                glm::vec3 p = glm::vec3(x, y, z) * radius;
                glm::vec3 n = glm::normalize(glm::vec3(x, y, z));

                Vertex vtx{};
                vtx.position = p;
                vtx.normal   = n;               // 填法线
                vtx.uv       = glm::vec2(u, v); // UV 简单映射
                vtx.color    = glm::vec4(       // 给一个明显的渐变颜色
                                         0.3f + 0.7f * u,
                                         0.3f + 0.7f * v,
                                         1.0f,
                                         1.0f
                                        );

                data.vertices.push_back(vtx);
            }
        }

        const uint32_t rowVerts = slices + 1;
        for (uint32_t i = 0; i < stacks; ++i)
        {
            for (uint32_t j = 0; j < slices; ++j)
            {
                uint32_t i0 = i * rowVerts + j;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (i + 1) * rowVerts + j;
                uint32_t i3 = i2 + 1;

                // 三角形 1
                data.indices.push_back(i0);
                data.indices.push_back(i2);
                data.indices.push_back(i1);

                // 三角形 2
                data.indices.push_back(i1);
                data.indices.push_back(i2);
                data.indices.push_back(i3);
            }
        }

        return data;
    }
    

     
    MeshData generateTriangleMesh()
    {
        MeshData data{};
     
        data.vertices.resize(3);
     
        // 顶点 0：底部中心
        data.vertices[0].position = glm::vec3(0.0f, -0.5f, 0.0f);
        data.vertices[0].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
        data.vertices[0].uv       = glm::vec2(0.5f, 0.0f);
        data.vertices[0].color    = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // 红
     
        // 顶点 1：右上
        data.vertices[1].position = glm::vec3(0.5f, 0.5f, 0.0f);
        data.vertices[1].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
        data.vertices[1].uv       = glm::vec2(1.0f, 1.0f);
        data.vertices[1].color    = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // 绿
     
        // 顶点 2：左上
        data.vertices[2].position = glm::vec3(-0.5f, 0.5f, 0.0f);
        data.vertices[2].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
        data.vertices[2].uv       = glm::vec2(0.0f, 1.0f);
        data.vertices[2].color    = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // 蓝
     
        // 非索引绘制的话 indices 可以为空，这里给一个简单索引
        data.indices = {0, 1, 2};
     
        return data;
    }
    
} // namespace
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
    {
        // 用内部函数生成球的顶点/索引，并交给 Renderer 创建 GPU mesh
        auto sphereData = generateTriangleMesh();
        sphereMesh_     = renderer_->createMesh(
                                                sphereData.vertices.data(),
                                                sphereData.vertices.size(),
                                                sphereData.indices.data(),
                                                sphereData.indices.size());

        sphereRenderable_.mesh     = sphereMesh_;
        sphereRenderable_.material = {}; // 现在暂时没用材质
        sphereInitialized_         = true;
    }

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
        if (sphereInitialized_)
        {
            renderer_->submitRenderables(&sphereRenderable_, 1);
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
