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
│   │   └── Renderer.h              # 渲染器基类
│   └── vulkan_backend/             # Vulkan 后端实现
│       ├── VulkanDevice.h          # Vulkan 设备封装
│       ├── VulkanRenderer.h        # Vulkan 渲染器实现
│       ├── ResourceManager.h       # GPU 资源管理器
│       ├── DescriptorSetManager.h  # 描述符集管理器
│       ├── SwapchainResources.h    # 交换链资源 RAII 封装
│       ├── vulkan_init.h           # Vulkan 初始化函数
│       ├── swapchain_management.h  # 交换链管理函数
│       ├── Rendering.h             # 渲染通道和管线函数
│       ├── command_buffer_sync.h   # 命令缓冲和同步函数
│       └── utils.h                 # Vulkan 工具函数
├── src/                            # 源代码目录
│   ├── core/                       # 核心模块实现
│   │   └── constants.cpp           # 全局常量定义
│   ├── platform/                   # 平台模块实现
│   │   ├── main.cpp                # 程序入口点
│   │   └── VulkanApp.cpp           # 应用程序实现
│   ├── renderer/                   # 渲染器模块（预留）
│   └── vulkan_backend/             # Vulkan 后端实现
│       ├── VulkanDevice.cpp        # Vulkan 设备实现
│       ├── VulkanRenderer.cpp      # Vulkan 渲染器实现
│       ├── ResourceManager.cpp     # GPU 资源管理实现
│       ├── DescriptorSetManager.cpp# 描述符集管理实现
│       ├── SwapchainResources.cpp  # 交换链资源实现
│       ├── vulkan_init.cpp         # Vulkan 初始化实现
│       ├── swapchain_management.cpp# 交换链管理实现
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
└── README.md                       # 项目说明
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

### Vulkan Backend 模块 (`include/vulkan_backend/`, `src/vulkan_backend/`)

Vulkan 后端实现，包含所有 Vulkan 相关代码：

#### 核心类
- **VulkanDevice**: 封装物理设备、逻辑设备和队列管理
- **VulkanRenderer**: 实现 Renderer 接口的 Vulkan 渲染器
- **ResourceManager**: 统一管理缓冲区、图像、采样器等 GPU 资源
- **DescriptorSetManager**: 管理描述符池和描述符集分配
- **SwapchainResources**: RAII 封装交换链及相关资源

#### 工具函数
- **vulkan_init**: Vulkan 实例、表面、设备创建
- **swapchain_management**: 交换链和图像视图创建
- **Rendering**: 渲染通道、管线、帧缓冲创建
- **command_buffer_sync**: 命令缓冲、同步原语、帧绘制
- **utils**: 设备选择、队列族查询、格式选择等工具函数

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