# Vulkan 学习项目：从 Hello Triangle 到可重建 Swapchain

简短描述：本仓库为一个基于 C++ + Vulkan 的学习/实验工程，目标是从基础的 Hello Triangle 演进到具备可重建 swapchain 的更工程化渲染框架。使用 GLFW 做窗口抽象，代码模块化，逐步引入 RAII 与资源聚合设计。

目录
- [功能概览](#功能概览)
- [项目结构](#项目结构)
- [构建与运行](#构建与运行)
- [窗口调整与 Swapchain 重建](#窗口调整与-swapchain-重建)
- [开发路线图](#开发路线图)
- [参考资料](#参考资料)
- [贡献与许可](#贡献与许可)

## 功能概览

主要实现点：
- Vulkan 初始化（VkInstance / 可选验证层 / VkSurfaceKHR / 物理设备选择 / 逻辑设备与队列）。
- Swapchain 管理与窗口大小调整（完全支持 Swapchain 的销毁与重建）。
- 将所有与窗口尺寸相关的资源聚合到 `SwapchainResources`：
  - `VkSwapchainKHR`、swapchain images & image views、`VkRenderPass`、`VkPipelineLayout`、`VkPipeline`、`VkFramebuffer`、与之对应的 `VkCommandBuffer` 列表。
- **动态渲染管线（Dynamic Pipeline）**：
  - 支持动态视口（Viewport）和裁剪矩形（Scissor）
  - 支持动态线宽（Line Width）和深度偏移（Depth Bias）
  - 在命令缓冲录制时动态设置管线状态，提高灵活性
- 渲染流程：
  - 通过独立模块创建 render pass 与 graphics pipeline（使用预编译 SPIR-V）。
  - 为每个 swapchain image 创建 framebuffer，并录制对应命令缓冲。
  - 使用两个信号量（image-available / render-finished）同步 acquire → submit → present。
  - drawFrame 封装一帧流程，并在遇到 VK_ERROR_OUT_OF_DATE_KHR / VK_SUBOPTIMAL_KHR 时返回上层以触发重建。
- 模块化代码组织：实例/设备初始化、swapchain 管理、渲染、命令缓冲与同步、工具函数等职责分离。

## 项目结构（概要）
- src/
  - main.cpp：程序入口，构造并运行 Application。
  - VulkanApp.cpp：Application 方法实现（窗口、Vulkan 生命周期、主循环、清理）。
  - vulkan_init.cpp：实例/调试/设备/队列初始化。
  - swapchain_management.cpp：swapchain 与 image view 的创建与销毁。
  - rendering.cpp：render pass / pipeline / framebuffer 创建。
  - command_buffer_sync.cpp：command pool / command buffers / semaphores / drawFrame。
  - SwapchainResources.cpp（可选）：管理 swapchain 相关资源的 RAII 实现。
- include/
  - Application.h、vulkan_init.h、swapchain_management.h、rendering.h、command_buffer_sync.h、SwapchainResources.h、Platform.h、constants.h
- shaders/: 预编译 SPIR-V（shader.vert.spv / shader.frag.spv）
- 构建脚本：CMakeLists.txt、build.bat / simple_build.bat（Windows）

## 构建与运行

依赖
- Vulkan SDK（建议 1.3+）
- GLFW
- CMake（建议 3.20+）
- C++17+ 编译器

Windows 快速构建示例：
```bash
# 使用仓库自带脚本（如存在）
build.bat

# 或手动
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

运行：在生成目录（例如 build/Debug/）执行生成的可执行文件。

## 窗口调整与 Swapchain 重建（要点）

关键思想：
- GLFW 仅在 framebuffer 大小变化时设置一个标志；实际的 swapchain 重建在主线程 / 主循环中安全完成。
- 重建流程需要先 vkDeviceWaitIdle，再销毁旧的与窗口相关资源并按顺序重建。

回调示例（在 initWindow 注册）：
```c++
// framebuffer 大小回调：仅设置标志，避免在回调中做 Vulkan 操作
void framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->framebufferResized = true;
}
```

主循环中的重建检测与渲染调用示例：
```c++
void Application::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (framebufferResized)
        {
            framebufferResized = false;
            createOrRecreateSwapchain(); // 销毁并重建所有窗口相关资源
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
```

createOrRecreateSwapchain 要点：
- 处理窗口最小化（framebuffer 尺寸为 0×0）情况：等待恢复到非零尺寸再创建 swapchain。
- 在销毁旧资源前调用 vkDeviceWaitIdle，确保没有命令在使用旧资源。
- 重建顺序通常为：swapchain → image views → render pass → pipeline → framebuffers → command buffers，并重新录制命令。

注意：正确处理 VK_ERROR_OUT_OF_DATE_KHR 与 VK_SUBOPTIMAL_KHR 返回码，确保在这些情况下触发重建。

## 开发路线图（简明）
- [x] **动态管线实现**（v0.2 已完成）
- [ ] 将 SwapchainResources 与 Application 解耦，提取 VulkanRenderer。
- [ ] 增加深度缓冲（Depth）与 MSAA 支持。
- [ ] 引入 descriptor sets 与 uniform buffers（矩阵、材质等）。
- [ ] 支持多对象渲染（多个三角形 / 网格）。
- [ ] 探索 Render Graph / Frame Graph 设计。

📘 **详细的工程化规划和路线图请参见 [PROJECT_PLAN.md](PROJECT_PLAN.md)**

## 调试与常见问题
- 确保 Vulkan SDK 安装且 VULKAN_SDK 环境变量正确。
- 在启用验证层时，关注控制台的 validation messages，它们能提示常见错误（同步、资源生命周期等）。
- 当窗口最小化导致 swapchain 创建失败时，检查并等待 framebuffer 大小恢复。

## 参考资料
- Vulkan 官方规范: https://registry.khronos.org/vulkan/specs/
- Vulkan Tutorial: https://vulkan-tutorial.com/
- Vulkan SDK samples（随 SDK 提供）

## 贡献与许可
欢迎基于本项目学习、反馈与贡献。建议先开 Issue 说明改动意图，再发 Pull Request。项目以学习为主，代码风格与设计会随学习进展演进。

