# Vulkan 学习项目：从 Hello Triangle 到可重建 Swapchain

本仓库是一个基于 C++ 和 Vulkan 的学习/实验项目，目标是从最基础的 “Hello Triangle” 一步步演进到更接近真实工程的渲染架构。
项目使用 GLFW 作为窗口系统抽象，采用模块化拆分（实例/设备初始化、Swapchain 管理、渲染管线、命令缓冲与同步等），并逐步引入 RAII
和资源聚合等现代 C++ 设计。

当前进度：已经实现了完整的窗口大小调整（Swapchain 重建）路径，并开始将与窗口尺寸相关的 Vulkan 资源聚合到
`SwapchainResources` 结构中，向更工程化的架构演进。
---

## 功能概览

目前项目已实现：

- 基本 Vulkan 初始化流程：
    - 创建 `VkInstance`，可选启用验证层。
    - 使用 GLFW 创建窗口并创建 `VkSurfaceKHR`。
    - 选择合适的物理设备（GPU）并创建逻辑设备 `VkDevice`。
    - 查询并创建图形队列和呈现队列（graphics queue / present queue）。

- Swapchain 管理与窗口调整：
    - 创建 `VkSwapchainKHR` 及其关联的 `VkImage`、`VkImageView`。
    - 将所有依赖窗口大小的资源聚合到 `SwapchainResources` 结构中（更贴近真实项目的资源管理方式）：
        - `VkSwapchainKHR`
        - Swapchain images & image views
        - `VkRenderPass`
        - `VkPipelineLayout` 和 `VkPipeline`
        - 每个 swapchain image 对应的 `VkFramebuffer`
        - 与之对应的 `VkCommandBuffer` 列表
    - 通过 GLFW 的 framebuffer 大小回调记录 `framebufferResized` 标志，在主循环中检测并调用 `createOrRecreateSwapchain()`
      重建整套与窗口尺寸相关的资源。
    - 处理窗口最小化为 0×0 的情况（等待窗口恢复到非零尺寸后再创建 swapchain）。

- 渲染管线与渲染流程：
    - 使用独立的 `rendering.*` 模块创建 `VkRenderPass` 和图形管线：
        - 固定功能阶段：视口、裁剪矩形、光栅化、混合等。
        - 着色器阶段：从预编译 SPIR-V (`shaders/shader.vert.spv` / `shaders/shader.frag.spv`) 创建 `VkShaderModule`。
    - 为每个 swapchain image view 创建对应的 `VkFramebuffer`。
    - 使用独立的 `command_buffer_sync.*` 模块：
        - 创建 `VkCommandPool` 并分配主命令缓冲。
        - 录制每个 framebuffer 对应的绘制命令（开始 render pass、绑定管线、绘制三角形、结束 render pass）。
        - 创建并使用两个 `VkSemaphore`（image-available / render-finished）进行图形队列与呈现队列之间的同步。
    - `drawFrame` 封装 acquire → submit → present 的整个一帧渲染流程，并在遇到 `VK_ERROR_OUT_OF_DATE_KHR` /
      `VK_SUBOPTIMAL_KHR` 时安全返回，交由上层驱动重建。

- 项目结构与模块化：
    - `Application` 类负责：
        - 整体生命周期控制：`run()` → `initWindow()` → `initVulkan()` → `mainLoop()` → `cleanup()`。
        - 管理 GLFW 窗口和 Vulkan 顶层对象（Instance / Surface / Device / Queues / CommandPool / Semaphores）。
        - 持有一个 `SwapchainResources` 实例，集中管理所有与窗口尺寸相关的 GPU 资源。
    - 各功能被拆分到独立头/源文件中，职责清晰：
        - `vulkan_init.*`：实例、调试、物理/逻辑设备与队列初始化。
        - `swapchain_management.*`：swapchain 及其 image views 的创建。
        - `rendering.*`：render pass / pipeline / framebuffer。
        - `command_buffer_sync.*`：command pool / command buffers / semaphores / drawFrame。
        - `utils.*`：通用辅助函数（如读取 SPIR-V 文件等）。
        - `constants.h`：窗口尺寸、校验层列表、设备扩展列表等全局常量。

 ---

## 代码结构

仓库的主要目录与文件：

- `src/`
    - `main.cpp`：程序入口，构造并运行 `Application`。
    - `VulkanApp.cpp`：`Application` 类的方法实现（窗口初始化、Vulkan 初始化、主循环、清理等）。
    - `vulkan_init.cpp`：实例、表面、物理/逻辑设备与队列的创建。
    - `swapchain_management.cpp`：swapchain 相关创建函数与 framebuffer 大小回调。
    - `rendering.cpp`：render pass / graphics pipeline / framebuffer 创建逻辑。
    - `command_buffer_sync.cpp`：command pool / command buffers / semaphores / drawFrame 实现。
    - （如存在）`SwapchainResources.cpp`：`SwapchainResources` 结构的实现（RAII 管理 swapchain 相关资源）。
- `include/`
    - `Application.h`：`Application` 类定义。
    - `vulkan_init.h` / `swapchain_management.h` / `rendering.h` / `command_buffer_sync.h`：与各模块对应的声明。
    - `SwapchainResources.h`：`SwapchainResources` 结构定义（聚合与窗口尺寸相关的 Vulkan 资源）。
    - `Platform.h`：平台相关包含（GLFW + Vulkan 头文件）。
    - `constants.h`：窗口尺寸、校验层与设备扩展等常量定义。
- `shaders/`
    - `shader.vert.spv` / `shader.frag.spv`：预编译顶点/片段着色器。
- 构建脚本
    - `CMakeLists.txt`：CMake 构建配置。
    - `build.bat` / `simple_build.bat` 等：Windows 下的构建脚本。

 ---

## 构建与运行

### 依赖

- Vulkan SDK（建议使用最新版本，至少 1.3+）
- GLFW
- CMake（推荐 3.20+）
- C++17 以上编译器（MSVC / Clang / GCC）

### 在 Windows 上构建

1. 安装 Vulkan SDK 并确保 `VULKAN_SDK` 环境变量已配置。
2. 在仓库根目录运行：

```bash
# 使用自带脚本（如果存在）
build.bat

# 或者手动执行
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

- 运行生成的可执行文件（通常在 build/Debug/ 或类似目录下）。

---
窗口大小调整与 Swapchain 重建流程

当前项目已经实现了完整的窗口调整处理链路，逻辑如下：

- GLFW 的 framebuffer 大小变化回调：
  - 在 `initWindow()` 中注册：  `glfwSetWindowUserPointer(window, this);`
    `glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);`
  - 回调函数中仅设置标志：  
```c++
void framebufferResizeCallback(GLFWwindow* window, int, int)
{
auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
if (app) app->framebufferResized = true;
}
```
      
- 主循环中检测并触发重建：  
```c++
void Application::mainLoop()
  {
      while (!glfwWindowShouldClose(window))
      {
          glfwPollEvents();
   
          if (framebufferResized)
          {
              framebufferResized = false;
              createOrRecreateSwapchain(); // 集中销毁并重建 swapchain 相关资源
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

- `createOrRecreateSwapchain()` 内部：
  - 处理窗口最小化为 0×0 的情况。
  - `vkDeviceWaitIdle(device)` 确保不会在使用旧资源时销毁它们。
  - 调用 `swapchainResources.destroy()` 清理旧的 framebuffer/pipeline/render pass/image view/swapchain。
  - 按与 initVulkan 中相同的顺序，重新创建 `swapchain` → `image views` → `render pass → graphics pipeline` → `framebuffers` → `command buffers`，并重新录制命令。

  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

接下来计划（Roadmap）

 - [ ]  将 SwapchainResources 与 Application 进一步解耦，重构为独立的 VulkanRenderer 类：
   - Application 只关心窗口和生命周期。
   - VulkanRenderer 管理 GPU 对象和渲染细节。
 - [ ]  为 swapchain 增加深度缓冲和 MSAA 支持。
 - [ ]  引入描述符集与 uniform 缓冲，支持带变换矩阵的场景。
 - [ ]  构建简单的多对象渲染示例（多个三角形 / 网格）。
 - [ ]  探索更高级的架构：Render Graph / Frame Graph。

   ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

学习资源

如果你对 Vulkan 还不熟悉，建议结合以下资料一起阅读本项目代码：

 - Vulkan 官方规范 (https://registry.khronos.org/vulkan/specs/): 全面的 API 参考（建议查阅具体章节）。
 - Vulkan Tutorial (https://vulkan-tutorial.com/): 与本项目结构较为接近的入门教程。
 - Vulkan SDK 自带 Samples：可以对照本工程理解常见使用模式。

   ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

本项目主要目的是学习与实践 Vulkan API 和现代 C++ 的工程化用法，因此代码会优先保证“结构清晰、易于演进”，再逐步引入更复杂的渲染技术与优化。 如果你有任何关于 Vulkan 结构设计、同步、性能优化方面的问题，欢迎在代码基础上继续提问和讨论。
