# Vulkan 学习项目 - 工程化规划文档
# Vulkan Study Project - Engineering Plan

[English](#english-version) | [中文](#中文版本)

---

## 中文版本

### 📋 项目概述

本项目是一个基于现代 C++ 和 Vulkan API 的学习型图形渲染框架，目标是从基础的三角形渲染逐步演进为一个工程化、模块化的渲染引擎原型。项目遵循现代 C++ 最佳实践（C++17/20），采用 RAII 资源管理模式，并逐步引入更高级的图形编程概念。

### 🎯 当前状态（v0.4）

#### 已实现的功能
- ✅ **Vulkan 核心初始化**
  - VkInstance 创建与验证层支持
  - 物理设备选择与逻辑设备创建
  - 队列族查找与队列获取
  - VkSurfaceKHR 创建（GLFW 集成）

- ✅ **交换链管理**
  - 交换链创建与销毁
  - 窗口大小变化时的交换链重建
  - SwapchainResources RAII 封装
  - 窗口最小化处理

- ✅ **渲染管线**
  - Render Pass 创建（单个颜色附件）
  - **动态图形管线**（Dynamic Pipeline）
    - 动态视口（Viewport）
    - 动态裁剪矩形（Scissor）
    - 动态线宽（Line Width）
    - 动态深度偏移（Depth Bias）
  - 基础光栅化状态
  - 颜色混合配置

- ✅ **命令缓冲与同步**
  - 命令池创建
  - 命令缓冲分配与录制
  - 基于信号量的 CPU-GPU 同步
  - 每帧渲染循环

- ✅ **着色器系统**
  - SPIR-V 着色器加载
  - 顶点和片段着色器支持
  - CMake 自动编译着色器

- ✅ **渲染器抽象层**（Phase 1.3 - v0.4 新增）
  - Renderer 抽象接口定义
  - VulkanRenderer 具体实现
  - Application 与渲染后端解耦
  - 支持未来多后端扩展（DX12/Metal）

#### 代码架构
```
vulkan_study/
├── include/              # 公共头文件
│   ├── Application.h     # 主应用类（与渲染后端解耦）
│   ├── Renderer.h        # 渲染器抽象接口
│   ├── VulkanRenderer.h  # Vulkan 渲染器实现
│   ├── VulkanDevice.h    # Vulkan 设备封装
│   ├── ResourceManager.h # 资源管理器
│   ├── DescriptorSetManager.h  # 描述符集管理
│   ├── vulkan_init.h     # Vulkan 初始化
│   ├── swapchain_management.h  # 交换链管理
│   ├── Rendering.h       # 渲染管线
│   ├── command_buffer_sync.h   # 命令与同步
│   ├── SwapchainResources.h    # 交换链资源 RAII
│   ├── constants.h       # 全局常量
│   ├── utils.h           # 工具函数
│   └── Platform.h        # 平台相关定义
├── src/                  # 实现文件
│   ├── main.cpp
│   ├── VulkanApp.cpp
│   ├── VulkanRenderer.cpp     # Vulkan 渲染器实现
│   ├── VulkanDevice.cpp       # 设备管理实现
│   ├── ResourceManager.cpp    # 资源管理实现
│   ├── DescriptorSetManager.cpp  # 描述符管理实现
│   ├── vulkan_init.cpp
│   ├── swapchain_management.cpp
│   ├── Rendering.cpp
│   ├── command_buffer_sync.cpp
│   ├── SwapchainResources.cpp
│   ├── constants.cpp
│   └── utils.cpp
├── shaders/              # GLSL 着色器源码
│   ├── shader.vert       # 顶点着色器
│   └── shader.frag       # 片段着色器
├── CMakeLists.txt        # CMake 构建配置
└── vcpkg.json            # 依赖管理
```

---

### 🚀 工程化改进路线图

#### 第一阶段：架构重构与模块化 ✅ **已完成**

**目标**：将代码重构为更清晰的层次结构，分离关注点，提高可维护性。

##### 1.1 动态管线增强 ✅ **已完成**
- [x] 将动态状态配置从头文件移到实现文件
- [x] 在命令缓冲录制时正确设置动态状态
- [x] 添加线宽和深度偏移动态状态支持
- [x] 文档化动态状态的使用方式和限制

##### 1.2 资源管理改进 ✅ **已完成**
- [x] **VulkanDevice 类**
  - 封装物理设备、逻辑设备、队列
  - 提供设备能力查询接口
  - 管理设备特性和扩展
  
- [x] **ResourceManager 类**
  - 统一管理 Buffer、Image、Sampler 等资源
  - 实现资源池和重用机制
  - 提供 RAII 风格的资源句柄

- [x] **DescriptorSetManager**
  - 管理 Descriptor Pool 和 Descriptor Set
  - 提供简化的描述符分配接口

##### 1.3 渲染抽象层 ✅ **已完成**
- [x] **Renderer 接口**
  - 定义渲染器的公共接口
  - 将来支持多后端（Vulkan/DX12/Metal）
  - 定义 API 无关的数据结构（FrameContext、CameraData、MeshHandle 等）
  - 提供帧生命周期管理接口（beginFrame、renderFrame、waitIdle）
  - 提供资源创建接口（createMesh、destroyMesh）
  - 提供场景提交接口（submitCamera、submitRenderables）
  
- [x] **VulkanRenderer 实现**
  - 从 Application 中分离渲染逻辑
  - 管理渲染循环和帧同步
  - 提供场景提交接口
  - 集成 VulkanDevice、ResourceManager、DescriptorSetManager
  - 实现完整的 Vulkan 渲染管线
  - 支持交换链重建和窗口调整

#### 第二阶段：核心渲染特性扩展

##### 2.1 顶点数据支持
- [ ] 定义 Vertex 结构体（位置、颜色、法线、UV）
- [ ] 实现顶点缓冲和索引缓冲创建
- [ ] 更新管线配置以支持顶点输入
- [ ] 加载简单的几何模型（立方体、球体）

##### 2.2 Uniform Buffer 和 Descriptor Sets
- [ ] 创建 UBO 用于传递变换矩阵
- [ ] 实现 MVP 矩阵计算（GLM 集成）
- [ ] 配置 Descriptor Set Layout
- [ ] 在着色器中使用 uniform 数据

##### 2.3 纹理和采样器
- [ ] 实现纹理图像加载（stb_image）
- [ ] 创建 Image View 和 Sampler
- [ ] 更新 Descriptor Sets 以包含纹理
- [ ] 在片段着色器中进行纹理采样

##### 2.4 深度测试和模板测试
- [ ] 创建深度缓冲图像
- [ ] 更新 Render Pass 以包含深度附件
- [ ] 启用深度测试
- [ ] 为未来效果预留模板测试支持

##### 2.5 多重采样抗锯齿（MSAA）
- [ ] 创建 MSAA 颜色和深度附件
- [ ] 更新 Render Pass 和 Framebuffer
- [ ] 配置管线的多重采样状态

#### 第三阶段：高级渲染技术

##### 3.1 多 Pass 渲染
- [ ] 实现离屏渲染到纹理
- [ ] 支持多个 Render Pass
- [ ] 实现简单的后处理效果（如 Bloom、色调映射）

##### 3.2 阴影映射
- [ ] 实现定向光阴影
- [ ] 使用深度偏移防止阴影失真
- [ ] 实现 PCF 软阴影

##### 3.3 PBR 材质系统
- [ ] 实现基于物理的光照模型
- [ ] 支持金属度-粗糙度工作流
- [ ] IBL（基于图像的光照）

##### 3.4 Compute Shader 支持
- [ ] 创建 Compute Pipeline
- [ ] 实现简单的计算任务（如粒子系统）
- [ ] CPU-GPU 数据传输优化

#### 第四阶段：性能优化与工具

##### 4.1 Render Graph / Frame Graph
- [ ] 设计 Render Graph 架构
- [ ] 自动资源生命周期管理
- [ ] 依赖关系跟踪和优化

##### 4.2 性能分析与优化
- [ ] 集成 Vulkan 时间戳查询
- [ ] 实现 GPU 性能计数器
- [ ] 优化批处理和实例化渲染

##### 4.3 调试与验证
- [ ] 增强验证层使用
- [ ] 集成 RenderDoc 或类似工具
- [ ] 实现自定义调试标记和标签

##### 4.4 跨平台支持
- [ ] Windows（已支持）
- [ ] Linux 支持
- [ ] 可选：macOS（通过 MoltenVK）

#### 第五阶段：引擎化与编辑器支持

##### 5.1 场景管理
- [ ] 实现场景图（Scene Graph）
- [ ] 节点层级和变换管理
- [ ] 相机系统（透视/正交）

##### 5.2 ECS（实体组件系统）探索
- [ ] 评估 ECS 架构的适用性
- [ ] 实现轻量级 ECS 或集成第三方库

##### 5.3 资产管线
- [ ] 模型导入（Assimp 或 TinyGLTF）
- [ ] 纹理压缩和流式加载
- [ ] 着色器热重载

##### 5.4 简单编辑器（可选）
- [ ] ImGui 集成
- [ ] 场景层级视图
- [ ] 材质和对象属性编辑器

---

### 📐 工程化规范与最佳实践

#### 代码风格

**命名约定**
- 类名：PascalCase（如 `VulkanDevice`, `RenderPass`）
- 函数/方法：camelCase（如 `createPipeline()`, `destroyResources()`）
- 成员变量：camelCase（如 `swapchain`, `commandPool`）
- 常量：UPPER_SNAKE_CASE（如 `MAX_FRAMES_IN_FLIGHT`）
- 私有成员可选前缀 `m_`（如 `m_device`，根据团队偏好）

**文件组织**
- 一个类一个头文件和实现文件
- 头文件使用 `#pragma once`
- 实现文件只包含必要的头文件
- 将平台相关代码隔离到 Platform.h/cpp

**注释规范**
- 公共接口使用 Doxygen 风格注释
- 复杂算法或 Vulkan 特定概念添加解释性注释
- 使用中英文双语注释，便于国际交流

#### RAII 资源管理

**原则**
- 每个 Vulkan 对象应有明确的所有者
- 使用 RAII 封装（构造时创建，析构时销毁）
- 避免裸指针和手动 `delete`，使用智能指针

**示例**
```cpp
class VulkanBuffer {
public:
    VulkanBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage);
    ~VulkanBuffer() { destroy(); }
    
    // 禁止拷贝，允许移动
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;
    
    VkBuffer getHandle() const { return m_buffer; }
    
private:
    void destroy();
    VkDevice m_device;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};
```

#### 错误处理

**策略**
- Vulkan API 调用后立即检查 VkResult
- 初始化失败使用异常（`std::runtime_error`）
- 运行时可恢复错误使用返回码或 `std::optional`
- 关键路径避免异常，使用错误码

**示例**
```cpp
// 初始化阶段 - 使用异常
void createBuffer(VkDevice device, VkDeviceSize size, VkBuffer& buffer) {
    VkBufferCreateInfo bufferInfo{};
    // ... 填充结构体
    
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer: " + std::to_string(result));
    }
}

// 渲染循环 - 使用返回码
bool acquireNextImage(uint32_t& imageIndex) {
    VkResult result = vkAcquireNextImageKHR(/*...*/);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false; // 需要重建交换链
    }
    // 处理其他错误...
    imageIndex = /*...*/;
    return true;
}
```

#### 同步与并发

**Vulkan 同步原语**
- **Fence**: CPU-GPU 同步（等待帧完成）
- **Semaphore**: GPU-GPU 同步（队列间、交换链）
- **Barrier**: 内存依赖和布局转换

**最佳实践**
- 使用多个帧重叠渲染（Frame-in-Flight）
- 避免 `vkQueueWaitIdle`，使用 Fence 细粒度同步
- 正确使用管线屏障（Pipeline Barrier）管理资源状态转换

#### 内存管理

**策略**
- 使用 Vulkan Memory Allocator (VMA) 简化内存分配
- 批量分配，减少内存碎片
- 为不同用途的资源使用不同的内存池（静态几何、动态 UBO、暂存）

**未来考虑**
- 流式纹理加载
- 资源淘汰和重用
- GPU 驻留内存管理

#### 着色器开发

**工作流程**
- GLSL 源码存放在 `shaders/` 目录
- CMake 自动编译为 SPIR-V
- 使用 `#include` 共享着色器代码
- 考虑使用 SPIRV-Reflect 进行反射

**工具**
- glslc（LunarG SDK）或 glslangValidator
- SPIRV-Tools 用于优化和验证
- RenderDoc 用于调试

#### 构建系统

**CMake 配置**
- 使用现代 CMake（3.20+）
- 目标驱动（target-based）依赖管理
- 支持多配置生成器（MSVC、Ninja）
- 集成 vcpkg 或 Conan 管理依赖

**依赖管理**
- Vulkan SDK（必需）
- GLFW（窗口和输入）
- GLM（数学库）
- stb_image（纹理加载）
- Assimp 或 TinyGLTF（模型加载，可选）
- VMA（内存分配，推荐）

#### 文档与维护

**文档类型**
- README.md：快速开始和概述
- PROJECT_PLAN.md：本文档，工程规划
- ProjectStructure.md：详细的模块说明
- 代码内注释：API 和设计决策

**版本控制**
- 使用语义化版本（Semantic Versioning）
- 主分支保持稳定
- 功能开发使用特性分支
- 及时更新 CHANGELOG.md

---

### 🔧 开发工具与环境

#### 必需工具
- **C++ 编译器**：MSVC 2019+, GCC 10+, Clang 11+
- **CMake**：3.20 或更高版本
- **Vulkan SDK**：1.3+ (LunarG)
- **Git**：版本控制

#### 推荐工具
- **IDE**: Visual Studio 2022, CLion, VS Code with C++ extensions
- **调试器**: RenderDoc, Nsight Graphics (NVIDIA), Radeon GPU Profiler (AMD)
- **分析器**: Tracy Profiler, Optick
- **着色器编辑器**: Visual Studio Code with GLSL extensions

#### 依赖库版本
```json
{
  "dependencies": {
    "glfw3": "^3.3.8",
    "glm": "^0.9.9",
    "stb": "latest",
    "vulkan": "^1.3.0"
  }
}
```

---

### 📚 学习资源

#### 官方文档
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/1.3/html/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)

#### 教程与书籍
- [Vulkan Tutorial](https://vulkan-tutorial.com/) - 本项目的起点
- *Vulkan Programming Guide* by Graham Sellers
- [Learn OpenGL](https://learnopengl.com/) - 图形编程基础

#### 社区与论坛
- [Khronos Vulkan Forum](https://community.khronos.org/c/vulkan/15)
- [r/vulkan](https://www.reddit.com/r/vulkan/)
- Discord: Vulkan 相关服务器

---

### 🤝 贡献指南

#### 如何贡献
1. Fork 本仓库
2. 创建特性分支（`git checkout -b feature/AmazingFeature`）
3. 遵循代码规范提交更改
4. 编写或更新测试（如适用）
5. 提交 Pull Request

#### 代码审查标准
- 代码符合项目风格指南
- 无明显性能问题
- 通过验证层检查
- 包含必要的注释和文档

#### 议题与 Bug 报告
- 使用 Issue 模板
- 提供复现步骤和环境信息
- 附上相关日志和截图

---

### 📜 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

---

### 📞 联系方式

- **项目维护者**: [GitHub](https://github.com/66six11/vulkan_study)
- **问题反馈**: 请使用 GitHub Issues

---

## English Version

### 📋 Project Overview

This is a learning-oriented graphics rendering framework based on modern C++ and the Vulkan API. The goal is to evolve from basic triangle rendering to an engineering-grade, modular rendering engine prototype. The project follows modern C++ best practices (C++17/20), employs RAII resource management, and gradually introduces advanced graphics programming concepts.

### 🎯 Current Status (v0.4)

#### Implemented Features
- ✅ **Vulkan Core Initialization**
  - VkInstance creation with validation layer support
  - Physical device selection and logical device creation
  - Queue family discovery and queue retrieval
  - VkSurfaceKHR creation (GLFW integration)

- ✅ **Swapchain Management**
  - Swapchain creation and destruction
  - Swapchain recreation on window resize
  - SwapchainResources RAII wrapper
  - Window minimization handling

- ✅ **Rendering Pipeline**
  - Render Pass creation (single color attachment)
  - **Dynamic Graphics Pipeline**
    - Dynamic Viewport
    - Dynamic Scissor
    - Dynamic Line Width
    - Dynamic Depth Bias
  - Basic rasterization state
  - Color blending configuration

- ✅ **Command Buffers & Synchronization**
  - Command pool creation
  - Command buffer allocation and recording
  - Semaphore-based CPU-GPU synchronization
  - Per-frame rendering loop

- ✅ **Shader System**
  - SPIR-V shader loading
  - Vertex and fragment shader support
  - CMake automatic shader compilation

- ✅ **Renderer Abstraction Layer** (Phase 1.3 - v0.4 New)
  - Renderer abstract interface definition
  - VulkanRenderer concrete implementation
  - Application decoupled from rendering backend
  - Support for future multi-backend expansion (DX12/Metal)

#### Code Architecture
*(Same as Chinese version - see above)*

---

### 🚀 Engineering Improvement Roadmap

#### Phase 1: Architecture Refactoring & Modularization ✅ **Completed**

**Goal**: Refactor code into clearer layers, separate concerns, improve maintainability.

##### 1.1 Dynamic Pipeline Enhancement ✅ **Completed**
- [x] Move dynamic state configuration from header to implementation
- [x] Properly set dynamic states during command buffer recording
- [x] Add line width and depth bias dynamic state support
- [x] Document dynamic state usage and limitations

##### 1.2 Resource Management Improvements ✅ **Completed**
- [x] **VulkanDevice Class**
  - Encapsulate physical device, logical device, queues
  - Provide device capability query interface
  - Manage device features and extensions
  
- [x] **ResourceManager Class**
  - Unified management of Buffer, Image, Sampler resources
  - Implement resource pooling and reuse
  - Provide RAII-style resource handles

- [x] **DescriptorSetManager**
  - Manage Descriptor Pool and Descriptor Sets
  - Provide simplified descriptor allocation interface

##### 1.3 Rendering Abstraction Layer ✅ **Completed**
- [x] **Renderer Interface**
  - Define public renderer interface
  - Future support for multiple backends (Vulkan/DX12/Metal)
  - Define API-agnostic data structures (FrameContext, CameraData, MeshHandle, etc.)
  - Provide frame lifecycle management interface (beginFrame, renderFrame, waitIdle)
  - Provide resource creation interface (createMesh, destroyMesh)
  - Provide scene submission interface (submitCamera, submitRenderables)
  
- [x] **VulkanRenderer Implementation**
  - Separate rendering logic from Application
  - Manage render loop and frame synchronization
  - Provide scene submission interface
  - Integrate VulkanDevice, ResourceManager, DescriptorSetManager
  - Implement complete Vulkan rendering pipeline
  - Support swapchain recreation and window resizing

#### Phase 2: Core Rendering Features Extension

##### 2.1 Vertex Data Support
- [ ] Define Vertex struct (position, color, normal, UV)
- [ ] Implement vertex buffer and index buffer creation
- [ ] Update pipeline configuration for vertex input
- [ ] Load simple geometric models (cube, sphere)

##### 2.2 Uniform Buffers and Descriptor Sets
- [ ] Create UBO for passing transformation matrices
- [ ] Implement MVP matrix calculation (GLM integration)
- [ ] Configure Descriptor Set Layout
- [ ] Use uniform data in shaders

##### 2.3 Textures and Samplers
- [ ] Implement texture image loading (stb_image)
- [ ] Create Image View and Sampler
- [ ] Update Descriptor Sets to include textures
- [ ] Perform texture sampling in fragment shader

##### 2.4 Depth Testing and Stencil Testing
- [ ] Create depth buffer image
- [ ] Update Render Pass to include depth attachment
- [ ] Enable depth testing
- [ ] Reserve stencil testing support for future effects

##### 2.5 Multisample Anti-Aliasing (MSAA)
- [ ] Create MSAA color and depth attachments
- [ ] Update Render Pass and Framebuffer
- [ ] Configure pipeline multisample state

#### Phase 3: Advanced Rendering Techniques

##### 3.1 Multi-Pass Rendering
- [ ] Implement offscreen rendering to texture
- [ ] Support multiple Render Passes
- [ ] Implement simple post-processing effects (Bloom, tone mapping)

##### 3.2 Shadow Mapping
- [ ] Implement directional light shadows
- [ ] Use depth bias to prevent shadow acne
- [ ] Implement PCF soft shadows

##### 3.3 PBR Material System
- [ ] Implement physically-based lighting model
- [ ] Support metallic-roughness workflow
- [ ] IBL (Image-Based Lighting)

##### 3.4 Compute Shader Support
- [ ] Create Compute Pipeline
- [ ] Implement simple compute tasks (particle system)
- [ ] Optimize CPU-GPU data transfer

#### Phase 4: Performance Optimization & Tools

##### 4.1 Render Graph / Frame Graph
- [ ] Design Render Graph architecture
- [ ] Automatic resource lifetime management
- [ ] Dependency tracking and optimization

##### 4.2 Performance Profiling & Optimization
- [ ] Integrate Vulkan timestamp queries
- [ ] Implement GPU performance counters
- [ ] Optimize batching and instanced rendering

##### 4.3 Debugging & Validation
- [ ] Enhanced validation layer usage
- [ ] Integrate RenderDoc or similar tools
- [ ] Implement custom debug markers and labels

##### 4.4 Cross-Platform Support
- [ ] Windows (already supported)
- [ ] Linux support
- [ ] Optional: macOS (via MoltenVK)

#### Phase 5: Engine & Editor Support

##### 5.1 Scene Management
- [ ] Implement Scene Graph
- [ ] Node hierarchy and transformation management
- [ ] Camera system (perspective/orthographic)

##### 5.2 ECS (Entity Component System) Exploration
- [ ] Evaluate ECS architecture suitability
- [ ] Implement lightweight ECS or integrate third-party library

##### 5.3 Asset Pipeline
- [ ] Model importing (Assimp or TinyGLTF)
- [ ] Texture compression and streaming
- [ ] Shader hot-reloading

##### 5.4 Simple Editor (Optional)
- [ ] ImGui integration
- [ ] Scene hierarchy view
- [ ] Material and object property editor

---

### 📐 Engineering Standards & Best Practices

#### Code Style

**Naming Conventions**
- Classes: PascalCase (e.g., `VulkanDevice`, `RenderPass`)
- Functions/Methods: camelCase (e.g., `createPipeline()`, `destroyResources()`)
- Member variables: camelCase (e.g., `swapchain`, `commandPool`)
- Constants: UPPER_SNAKE_CASE (e.g., `MAX_FRAMES_IN_FLIGHT`)
- Private members optional prefix `m_` (e.g., `m_device`, based on team preference)

**File Organization**
- One class per header and implementation file
- Use `#pragma once` in headers
- Include only necessary headers in implementation files
- Isolate platform-specific code to Platform.h/cpp

**Comment Standards**
- Public interfaces use Doxygen-style comments
- Add explanatory comments for complex algorithms or Vulkan-specific concepts
- Bilingual comments (Chinese/English) for international collaboration

#### RAII Resource Management

**Principles**
- Each Vulkan object should have a clear owner
- Use RAII wrapping (create on construction, destroy on destruction)
- Avoid raw pointers and manual `delete`, use smart pointers

**Example**
```cpp
class VulkanBuffer {
public:
    VulkanBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage);
    ~VulkanBuffer() { destroy(); }
    
    // Delete copy, allow move
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;
    
    VkBuffer getHandle() const { return m_buffer; }
    
private:
    void destroy();
    VkDevice m_device;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};
```

#### Error Handling

**Strategy**
- Immediately check VkResult after Vulkan API calls
- Use exceptions for initialization failures (`std::runtime_error`)
- Use return codes or `std::optional` for recoverable runtime errors
- Avoid exceptions in critical paths, use error codes

**Example**
```cpp
// Initialization phase - use exceptions
void createBuffer(VkDevice device, VkDeviceSize size, VkBuffer& buffer) {
    VkBufferCreateInfo bufferInfo{};
    // ... fill structure
    
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer: " + std::to_string(result));
    }
}

// Render loop - use return codes
bool acquireNextImage(uint32_t& imageIndex) {
    VkResult result = vkAcquireNextImageKHR(/*...*/);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false; // Need to recreate swapchain
    }
    // Handle other errors...
    imageIndex = /*...*/;
    return true;
}
```

#### Synchronization & Concurrency

**Vulkan Synchronization Primitives**
- **Fence**: CPU-GPU synchronization (wait for frame completion)
- **Semaphore**: GPU-GPU synchronization (between queues, swapchain)
- **Barrier**: Memory dependencies and layout transitions

**Best Practices**
- Use multiple frames in flight for overlapped rendering
- Avoid `vkQueueWaitIdle`, use Fence for fine-grained synchronization
- Properly use Pipeline Barriers to manage resource state transitions

#### Memory Management

**Strategy**
- Use Vulkan Memory Allocator (VMA) to simplify memory allocation
- Batch allocations to reduce fragmentation
- Use different memory pools for different resource purposes (static geometry, dynamic UBO, staging)

**Future Considerations**
- Streaming texture loading
- Resource eviction and reuse
- GPU resident memory management

#### Shader Development

**Workflow**
- GLSL source code in `shaders/` directory
- CMake automatically compiles to SPIR-V
- Use `#include` for shared shader code
- Consider using SPIRV-Reflect for reflection

**Tools**
- glslc (LunarG SDK) or glslangValidator
- SPIRV-Tools for optimization and validation
- RenderDoc for debugging

#### Build System

**CMake Configuration**
- Use modern CMake (3.20+)
- Target-based dependency management
- Support multi-config generators (MSVC, Ninja)
- Integrate vcpkg or Conan for dependency management

**Dependency Management**
- Vulkan SDK (required)
- GLFW (window and input)
- GLM (math library)
- stb_image (texture loading)
- Assimp or TinyGLTF (model loading, optional)
- VMA (memory allocation, recommended)

#### Documentation & Maintenance

**Documentation Types**
- README.md: Quick start and overview
- PROJECT_PLAN.md: This document, engineering plan
- ProjectStructure.md: Detailed module description
- In-code comments: API and design decisions

**Version Control**
- Use Semantic Versioning
- Keep main branch stable
- Use feature branches for development
- Update CHANGELOG.md regularly

---

### 🔧 Development Tools & Environment

#### Required Tools
- **C++ Compiler**: MSVC 2019+, GCC 10+, Clang 11+
- **CMake**: 3.20 or higher
- **Vulkan SDK**: 1.3+ (LunarG)
- **Git**: Version control

#### Recommended Tools
- **IDE**: Visual Studio 2022, CLion, VS Code with C++ extensions
- **Debugger**: RenderDoc, Nsight Graphics (NVIDIA), Radeon GPU Profiler (AMD)
- **Profiler**: Tracy Profiler, Optick
- **Shader Editor**: Visual Studio Code with GLSL extensions

#### Dependency Versions
```json
{
  "dependencies": {
    "glfw3": "^3.3.8",
    "glm": "^0.9.9",
    "stb": "latest",
    "vulkan": "^1.3.0"
  }
}
```

---

### 📚 Learning Resources

#### Official Documentation
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/1.3/html/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)

#### Tutorials & Books
- [Vulkan Tutorial](https://vulkan-tutorial.com/) - Starting point for this project
- *Vulkan Programming Guide* by Graham Sellers
- [Learn OpenGL](https://learnopengl.com/) - Graphics programming fundamentals

#### Community & Forums
- [Khronos Vulkan Forum](https://community.khronos.org/c/vulkan/15)
- [r/vulkan](https://www.reddit.com/r/vulkan/)
- Discord: Vulkan-related servers

---

### 🤝 Contribution Guidelines

#### How to Contribute
1. Fork this repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes following code standards
4. Write or update tests (if applicable)
5. Submit a Pull Request

#### Code Review Standards
- Code conforms to project style guide
- No obvious performance issues
- Passes validation layer checks
- Includes necessary comments and documentation

#### Issues & Bug Reports
- Use issue templates
- Provide reproduction steps and environment info
- Attach relevant logs and screenshots

---

### 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

### 📞 Contact

- **Project Maintainer**: [GitHub](https://github.com/66six11/vulkan_study)
- **Issue Reporting**: Please use GitHub Issues

---

## 更新日志 / Changelog

### v0.4 (Current) - 2025-11-23
- ✅ 实现 Renderer 抽象接口（支持未来多后端扩展）
- ✅ 实现 VulkanRenderer 类（完整的 Vulkan 渲染器实现）
- ✅ Application 与渲染后端解耦
- ✅ 完成阶段 1.3 渲染抽象层
- ✅ 完成第一阶段：架构重构与模块化

### v0.3
- ✅ 实现 VulkanDevice 类（封装物理设备、逻辑设备和队列管理）
- ✅ 实现 ResourceManager 类（统一管理 Buffer、Image、Sampler 资源）
- ✅ 实现 DescriptorSetManager 类（简化描述符集分配和管理）
- ✅ 完成阶段 1.2 资源管理改进

### v0.2
- ✅ 实现动态管线（Dynamic Pipeline）
- ✅ 添加项目工程化规划文档
- ✅ 改进代码注释和文档

### v0.1
- ✅ 基础 Vulkan 初始化
- ✅ Hello Triangle 渲染
- ✅ 交换链重建支持
- ✅ RAII 资源封装（SwapchainResources）

---

**最后更新 / Last Updated**: 2025-11-23
