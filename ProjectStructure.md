# Vulkan 项目结构说明

## 目录结构

```
Vulkan/
├── src/                    # 源代码文件 (.cpp)
│   ├── main.cpp            # 程序入口点
│   ├── HelloTriangleApplication.cpp  # 主应用程序实现
│   ├── vulkan_init.cpp     # Vulkan初始化实现
│   ├── swapchain_management.cpp  # 交换链管理实现
│   ├── rendering.cpp       # 渲染相关实现
│   ├── command_buffer_sync.cpp  # 命令缓冲和同步实现
│   └── utils.cpp           # 工具函数实现
├── include/                # 头文件 (.h)
│   ├── HelloTriangleApplication.h  # 主应用程序声明
│   ├── vulkan_init.h       # Vulkan初始化声明
│   ├── swapchain_management.h  # 交换链管理声明
│   ├── rendering.h         # 渲染相关声明
│   ├── command_buffer_sync.h  # 命令缓冲和同步声明
│   └── utils.h             # 工具函数声明
├── shaders/                # 着色器文件 (.vert, .frag, .comp)
├── assets/                 # 资源文件 (纹理, 模型等)
├── CMakeLists.txt          # CMake构建配置
├── CMake/                  # CMake模块
└── README.md               # 项目说明
```

## 文件组织说明

### src/ 目录
包含所有C++源代码文件(.cpp)，按功能模块组织：
- `main.cpp`: 程序入口点
- `HelloTriangleApplication.*`: 核心应用程序类
- `vulkan_init.*`: Vulkan初始化相关功能
- `swapchain_management.*`: 交换链管理功能
- `rendering.*`: 渲染通道和管线相关功能
- `command_buffer_sync.*`: 命令缓冲和同步功能
- `utils.*`: 辅助工具函数

### include/ 目录
包含所有头文件(.h)，与src目录中的源文件一一对应。

### shaders/ 目录
预留用于存放着色器文件，包括顶点着色器(.vert)、片段着色器(.frag)等。

### assets/ 目录
预留用于存放纹理、模型等资源文件。

## 构建说明

请确保已安装以下依赖：
- Vulkan SDK
- GLFW3 (通过vcpkg安装)
- CMake 3.20+

使用以下命令构建项目：
```bash
mkdir build
cd build
cmake ..
cmake --build .
```