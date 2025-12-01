# Vulkan 项目结构说明

## 目录结构

```
vulkan_study/
├── include/                        # 头文件目录
│   ├── core/                       # 核心工具和常量
│   │   ├── constants.h             # 全局常量定义
│   │   └── TimeStamp.h             # 时间戳工具类
│   ├── platform/                   # 平台相关代码
│   │   ├── Application.h           # 应用程序主类
│   │   └── Platform.h              # 平台抽象层（GLFW）
│   ├── renderer/                   # 渲染器抽象接口
│   │   ├── Renderer.h              # 渲染器基类
│   │   └── Vertex.h                # 顶点数据结构定义
│   └── vulkan_backend/             # Vulkan 后端实现
│       ├── VulkanDevice.h          # Vulkan 设备封装
│       ├── VulkanRenderer.h        # Vulkan 渲染器实现
│       ├── ResourceManager.h       # GPU 资源管理器
│       ├── DescriptorSetManager.h  # 描述符集管理器
│       ├── SwapchainResources.h    # 交换链资源 RAII 封装
│       ├── VertexInputDescription.h # 顶点输入布局描述
│       ├── vulkan_init.h           # Vulkan 初始化函数 (vkinit 命名空间)
│       ├── swapchain_management.h  # 交换链管理函数 (vkswapchain 命名空间)
│       ├── Rendering.h             # 渲染通道和管线函数 (vkpipeline 命名空间)
│       ├── command_buffer_sync.h   # 命令缓冲和同步函数 (vkcmd 命名空间)
│       └── utils.h                 # Vulkan 工具函数 (vkutil 命名空间)
├── src/                            # 源代码目录
│   ├── core/                       # 核心模块实现
│   │   └── constants.cpp           # 全局常量定义
│   ├── platform/                   # 平台模块实现
│   │   ├── main.cpp                # 程序入口点
│   │   └── VulkanApp.cpp           # 应用程序实现
│   └── vulkan_backend/             # Vulkan 后端实现
│       ├── VulkanDevice.cpp        # Vulkan 设备实现
│       ├── VulkanRenderer.cpp      # Vulkan 渲染器实现
│       ├── ResourceManager.cpp     # GPU 资源管理实现
│       ├── DescriptorSetManager.cpp # 描述符集管理实现
│       ├── SwapchainResources.cpp  # 交换链资源实现
│       ├── vulkan_init.cpp         # Vulkan 初始化实现
│       ├── swapchain_management.cpp # 交换链管理实现
│       ├── Rendering.cpp           # 渲染通道和管线实现
│       ├── command_buffer_sync.cpp # 命令缓冲和同步实现
│       └── utils.cpp               # Vulkan 工具函数实现
├── shaders/                        # 着色器文件
│   ├── shader.vert                 # 顶点着色器
│   └── shader.frag                 # 片段着色器
├── CMake/                          # CMake 模块
│   └── FindVulkan.cmake            # Vulkan 查找模块
├── CMakeLists.txt                  # CMake 构建配置
├── vcpkg.json                      # vcpkg 依赖配置
├── PROJECT_PLAN.md                 # 项目规划文档
├── ProjectStructure.md             # 项目结构文档（本文件）
└── README.md                       # 项目说明
```

## 命名空间组织

公共函数按功能分类到不同的命名空间中：

| 命名空间 | 文件 | 功能描述 |
|---------|------|---------|
| `vkutil` | utils.h/cpp | 设备选择、队列族查询、交换链配置等工具函数 |
| `vkinit` | vulkan_init.h/cpp | Vulkan 实例、表面、设备创建等初始化函数 |
| `vkswapchain` | swapchain_management.h/cpp | 交换链和图像视图管理函数 |
| `vkpipeline` | Rendering.h/cpp | 渲染通道、图形管线、帧缓冲、着色器模块创建函数 |
| `vkcmd` | command_buffer_sync.h/cpp | 命令池、命令缓冲、信号量、帧绘制函数 |
| `vkvertex` | VertexInputDescription.h | 顶点输入绑定和属性描述 |

### 使用示例

```cpp
// 初始化 Vulkan 实例和表面
vkinit::createInstance(instance, window);
vkinit::createSurface(instance, window, surface);

// 创建交换链
vkswapchain::createSwapChain(physicalDevice, device, surface, indices, ...);

// 创建渲染管线
vkpipeline::createRenderPass(device, imageFormat, renderPass);
vkpipeline::createGraphicsPipeline(device, extent, renderPass, layout, pipeline);

// 创建命令缓冲
vkcmd::createCommandPool(device, indices, commandPool);
vkcmd::createCommandBuffers(device, pool, framebuffers, ...);

// 查询设备特性
auto indices = vkutil::findQueueFamilies(device, surface);
bool suitable = vkutil::isDeviceSuitable(device, surface);

// 获取顶点输入描述
auto bindingDesc = vkvertex::getBindingDescription();
auto attrDescs = vkvertex::getAttributeDescriptions();
```

## 模块说明

### Core 模块 (`include/core/`, `src/core/`)

核心工具和常量定义模块，包含：
- **constants.h/cpp**: 全局常量（窗口尺寸、设备扩展、验证层等）
- **TimeStamp.h**: 时间戳工具类，用于帧计时

### Platform 模块 (`include/platform/`, `src/platform/`)

平台相关代码，负责窗口管理和应用程序生命周期：
- **Platform.h**: 平台抽象层，包含 GLFW 头文件
- **Application.h/VulkanApp.cpp**: 应用程序主类，管理窗口和渲染循环

### Renderer 模块 (`include/renderer/`)

渲染器抽象接口，定义与后端无关的渲染 API：
- **Renderer.h**: 渲染器基类，定义 initialize、resize、beginFrame、renderFrame 等接口
- **Vertex.h**: 顶点数据结构，包含位置（position）、法线（normal）、UV 坐标和颜色（color）

### Vulkan Backend 模块 (`include/vulkan_backend/`, `src/vulkan_backend/`)

Vulkan 后端实现，包含所有 Vulkan 相关代码：

#### 核心类
- **VulkanDevice**: 封装物理设备、逻辑设备和队列管理
- **VulkanRenderer**: 实现 Renderer 接口的 Vulkan 渲染器
- **ResourceManager**: 统一管理缓冲区、图像、采样器等 GPU 资源
- **DescriptorSetManager**: 管理描述符池和描述符集分配
- **SwapchainResources**: RAII 封装交换链及相关资源
- **VertexInputDescription**: 顶点输入布局描述（vkvertex 命名空间）

#### 工具函数（命名空间）
- **vkinit**: Vulkan 实例、表面、设备创建
- **vkswapchain**: 交换链和图像视图创建
- **vkpipeline**: 渲染通道、管线、帧缓冲创建
- **vkcmd**: 命令缓冲、同步原语、帧绘制
- **vkutil**: 设备选择、队列族查询、格式选择等工具函数

## 架构设计

```
┌──────────────────────────────────────────────────────────┐
│                      Application                          │
│                    (Platform Layer)                       │
├──────────────────────────────────────────────────────────┤
│                       Renderer                            │
│                   (Abstract Interface)                    │
├──────────────────────────────────────────────────────────┤
│                    VulkanRenderer                         │
│                  (Vulkan Backend)                         │
├──────────────────────────────────────────────────────────┤
│  VulkanDevice  │  ResourceManager  │  DescriptorSetManager│
│                │                   │                      │
│  SwapchainResources               │                      │
├──────────────────────────────────────────────────────────┤
│  vkinit │ vkswapchain │ vkpipeline │ vkcmd │ vkutil      │
│                    (Helper Functions)                     │
├──────────────────────────────────────────────────────────┤
│                    Vulkan SDK                             │
└──────────────────────────────────────────────────────────┘
```

## 构建说明

请确保已安装以下依赖：

- Vulkan SDK (LunarG)
- GLFW3 (通过 vcpkg 安装)
- CMake 3.20+
- C++20 兼容编译器

使用以下命令构建项目：

```bash
# 配置
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake

# 构建
cmake --build .
```

## 代码风格

- 使用 C++20 标准
- RAII 管理 Vulkan 资源
- 类名使用 PascalCase
- 函数和变量使用 camelCase
- 私有成员变量使用下划线后缀（如 `device_`）
- 头文件使用 `#pragma once`
- 公共函数按功能组织到命名空间中（如 `vkinit`、`vkutil` 等）