# Vulkan项目详解

## 项目概述

本项目是一个使用Vulkan API实现的简单三角形渲染程序。它演示了Vulkan的基本使用方法，包括窗口创建、Vulkan实例初始化、设备选择、交换链管理、渲染管线设置、命令缓冲记录和帧渲染等核心概念。

## 核心类和结构体

### Application类

Application类是整个程序的核心，负责管理Vulkan应用的所有资源。

#### 成员变量详解

##### 窗口和Vulkan实例相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `window` | `GLFWwindow*` | GLFW窗口对象，用于创建和管理应用程序窗口 |
| `instance` | `VkInstance` | Vulkan实例，是与Vulkan驱动程序交互的入口点 |
| `surface` | `VkSurfaceKHR` | 窗口表面，用于连接窗口系统和Vulkan |

##### 物理和逻辑设备相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `physicalDevice` | `VkPhysicalDevice` | 物理设备（GPU），代表系统中的实际图形硬件 |
| `device` | `VkDevice` | 逻辑设备，用于与GPU进行交互，是应用程序与物理设备通信的主要接口 |

##### 队列相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `graphicsQueue` | `VkQueue` | 图形队列，用于提交图形命令（如绘制操作） |
| `presentQueue` | `VkQueue` | 呈现队列，用于将图像呈现到屏幕 |

##### 交换链相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `swapChain` | `VkSwapchainKHR` | 交换链，用于管理呈现图像，实现双缓冲或三缓冲 |
| `swapChainImages` | `std::vector<VkImage>` | 交换链中的图像集合 |
| `swapChainImageFormat` | `VkFormat` | 交换链图像格式，定义图像中像素的存储格式 |
| `swapChainExtent` | `VkExtent2D` | 交换链图像尺寸（宽度和高度） |
| `swapChainImageViews` | `std::vector<VkImageView>` | 图像视图集合，用于访问图像数据 |

##### 渲染通道相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `renderPass` | `VkRenderPass` | 渲染通道，定义渲染操作的附件和子通道，描述渲染流程 |

##### 图形管线相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `pipelineLayout` | `VkPipelineLayout` | 管线布局，定义着色器使用的资源布局（如uniform缓冲区、采样器等） |
| `graphicsPipeline` | `VkPipeline` | 图形管线，定义图形渲染的完整状态（顶点输入、装配、光栅化、片段处理等） |

##### 帧缓冲相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `swapChainFramebuffers` | `std::vector<VkFramebuffer>` | 帧缓冲集合，用于存储渲染附件，每个帧缓冲对应一个交换链图像 |

##### 命令相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `commandPool` | `VkCommandPool` | 命令池，用于分配命令缓冲 |
| `commandBuffers` | `std::vector<VkCommandBuffer>` | 命令缓冲集合，用于记录命令序列 |

##### 同步相关

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `imageAvailableSemaphore` | `VkSemaphore` | 图像可用信号量，用于同步图像获取和渲染开始 |
| `renderFinishedSemaphore` | `VkSemaphore` | 渲染完成信号量，用于同步渲染完成和图像呈现 |

### 常量和全局变量

#### constants.h/cpp

| 变量名 | 类型 | 作用和用途 |
|--------|------|------------|
| `WIDTH` | `const uint32_t` | 窗口宽度（像素） |
| `HEIGHT` | `const uint32_t` | 窗口高度（像素） |
| `deviceExtensions` | `const std::vector<const char*>` | 需要启用的设备扩展列表，当前只包含交换链扩展 |

#### 结构体

##### SwapChainSupportDetails

封装物理设备对交换链的支持信息：

| 成员变量 | 类型 | 作用和用途 |
|----------|------|------------|
| `capabilities` | `VkSurfaceCapabilitiesKHR` | 表面能力，包括最小/最大图像数量、尺寸范围等 |
| `formats` | `std::vector<VkSurfaceFormatKHR>` | 支持的表面格式列表 |
| `presentModes` | `std::vector<VkPresentModeKHR>` | 支持的呈现模式列表 |

##### QueueFamilyIndices

用于存储图形队列和呈现队列的索引：

| 成员变量 | 类型 | 作用和用途 |
|----------|------|------------|
| `graphicsFamily` | `std::optional<uint32_t>` | 图形队列族索引，用于图形命令提交 |
| `presentFamily` | `std::optional<uint32_t>` | 呈现队列族索引，用于将图像呈现到屏幕 |
| `isComplete()` | 方法 | 检查是否找到了所有必需的队列族 |

## 核心函数详解

### Vulkan初始化相关 (vulkan_init.h/cpp)

#### createInstance()
创建Vulkan实例，这是使用Vulkan的第一步。

#### setupDebugMessenger()
设置调试信息回调（当前版本为空实现）。

#### createSurface()
创建窗口表面，用于连接窗口系统和Vulkan。

#### pickPhysicalDevice()
选择合适的物理设备（GPU）。

#### createLogicalDevice()
创建逻辑设备，用于与GPU进行交互。

### 交换链管理相关 (swapchain_management.h/cpp)

#### createSwapChain()
创建交换链，用于管理呈现图像。

#### createImageViews()
创建图像视图，用于访问图像数据。

### 渲染相关 (rendering.h/cpp)

#### createRenderPass()
创建渲染通道，定义渲染操作的附件和子通道。

#### createGraphicsPipeline()
创建图形管线，定义图形渲染的完整状态。

#### createFramebuffers()
创建帧缓冲，用于存储渲染附件。

#### createShaderModule()
创建着色器模块，用于加载着色器代码。

### 命令缓冲和同步相关 (command_buffer_sync.h/cpp)

#### createCommandPool()
创建命令池，用于分配命令缓冲。

#### createCommandBuffers()
创建命令缓冲，用于记录命令序列。

#### createSemaphores()
创建信号量，用于同步操作。

#### recordCommandBuffer()
记录命令缓冲，将渲染命令写入命令缓冲。

#### drawFrame()
绘制一帧，执行渲染流程。

### 工具函数 (utils.h/cpp)

#### isDeviceSuitable()
检查设备是否适合运行应用程序。

#### findQueueFamilies()
查找队列族索引。

#### checkDeviceExtensionSupport()
检查设备扩展支持情况。

#### querySwapChainSupport()
查询交换链支持详情。

#### chooseSwapSurfaceFormat()
选择交换链表面格式。

#### chooseSwapPresentMode()
选择交换链呈现模式。

#### chooseSwapExtent()
选择交换链图像尺寸。

## 着色器详解

### 顶点着色器 (shader.vert)

```glsl
#version 450

void main() {
    // 三角形的三个顶点 - 在标准化设备坐标中
    vec2 positions[3] = vec2[](
        vec2(0.0, -0.5),   // 底部中心
        vec2(0.5, 0.5),    // 右上角
        vec2(-0.5, 0.5)    // 左上角
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

该着色器定义了一个简单的三角形，三个顶点坐标在标准化设备坐标系中。

### 片段着色器 (shader.frag)

```glsl
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // 红色
}
```

该着色器将所有片段渲染为红色。

## 执行流程

1. `main()`函数创建Application实例并调用`run()`方法
2. `run()`方法依次调用：
   - `initWindow()`: 初始化GLFW窗口
   - `initVulkan()`: 初始化Vulkan相关对象
   - `mainLoop()`: 进入主循环，持续渲染直到窗口关闭
   - `cleanup()`: 清理所有分配的Vulkan资源

3. `initVulkan()`初始化流程：
   - 创建Vulkan实例
   - 设置调试信息回调
   - 创建窗口表面
   - 选择物理设备
   - 创建逻辑设备
   - 创建交换链及相关资源
   - 创建渲染通道
   - 创建图形管线
   - 创建帧缓冲
   - 创建命令池和命令缓冲
   - 创建同步对象
   - 记录命令缓冲

4. `mainLoop()`循环执行：
   - 处理窗口事件
   - 调用`drawFrame()`绘制每一帧

5. `cleanup()`按相反顺序销毁所有Vulkan对象，释放资源。

## Vulkan核心概念

### 实例(Instance)
Vulkan实例是应用程序与Vulkan驱动程序交互的入口点。它管理应用程序的全局状态，如启用的扩展和验证层。

### 物理设备(Physical Device)和逻辑设备(Device)
物理设备代表系统中的实际图形硬件（GPU）。逻辑设备是应用程序与物理设备通信的主要接口，通过它可以创建和管理图形资源。

### 队列(Queue)
队列是向GPU提交命令的方式。不同类型的队列支持不同的操作，如图形命令、计算命令和传输命令。

### 交换链(Swap Chain)
交换链用于管理呈现图像，实现双缓冲或三缓冲，避免画面撕裂。

### 渲染通道(Render Pass)
渲染通道定义渲染操作的附件和子通道，描述渲染流程。

### 图形管线(Graphics Pipeline)
图形管线定义图形渲染的完整状态，包括顶点输入、装配、光栅化、片段处理等阶段。

### 命令缓冲(Command Buffer)
命令缓冲用于记录命令序列，然后提交给队列执行。

### 同步(Synchronization)
Vulkan是显式同步的API，需要使用信号量、栅栏等同步原语来控制操作顺序。