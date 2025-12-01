# Vulkan 学习项目 / Vulkan Study Project

[中文](#中文) | [English](#english)

---

## 中文

一个基于现代 C++ 和 Vulkan API 的学习型图形渲染框架。目标是从基础的 Hello Triangle 演进为工程化、模块化的渲染引擎原型。

### ✨ 主要特性

- **现代 C++ 实践**：C++20 标准，RAII 资源管理
- **模块化架构**：渲染器抽象层，支持未来多后端扩展
- **完整 Vulkan 管线**：包含动态管线状态、交换链重建、同步管理
- **工程化设计**：清晰的代码分层，命名空间组织的工具函数

### 🚀 快速开始

#### 依赖

- **Vulkan SDK** 1.3+（[LunarG](https://vulkan.lunarg.com/)）
- **CMake** 3.20+
- **C++20 兼容编译器**（MSVC 2019+、GCC 10+、Clang 11+）
- **vcpkg**（用于管理 GLFW3 和 GLM 依赖）

#### 构建

```bash
# 克隆仓库
git clone https://github.com/66six11/vulkan_study.git
cd vulkan_study

# 配置与构建
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# 运行（Windows）
./bin/Release/vulkan.exe
```

或使用提供的脚本：
```bash
# Windows
build.bat
```

### 📚 文档

| 文档 | 说明 |
|------|------|
| [PROJECT_PLAN.md](PROJECT_PLAN.md) | 详细的工程规划、路线图和最佳实践 |
| [ProjectStructure.md](ProjectStructure.md) | 项目结构、模块说明和架构设计 |
| [Vulkan项目详解.md](Vulkan项目详解.md) | Vulkan 实现技术细节 |

### 🔧 当前版本：v0.4.1

**已完成**：
- ✅ 第一阶段：架构重构与模块化
- ✅ Renderer 抽象接口与 VulkanRenderer 实现
- ✅ VulkanDevice、ResourceManager、DescriptorSetManager
- ✅ 顶点数据结构定义（Vertex、VertexInputDescription）

**进行中**：
- 🔨 第二阶段：核心渲染特性扩展（顶点缓冲、UBO、纹理）

> 📘 完整路线图请参见 [PROJECT_PLAN.md](PROJECT_PLAN.md)

### 📜 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

## English

A learning-oriented graphics rendering framework based on modern C++ and Vulkan API. The goal is to evolve from a basic Hello Triangle to an engineering-grade, modular rendering engine prototype.

### ✨ Key Features

- **Modern C++ Practices**: C++20 standard, RAII resource management
- **Modular Architecture**: Renderer abstraction layer, supports future multi-backend expansion
- **Complete Vulkan Pipeline**: Dynamic pipeline states, swapchain recreation, synchronization management
- **Engineering Design**: Clear code layering, namespace-organized utility functions

### 🚀 Quick Start

#### Dependencies

- **Vulkan SDK** 1.3+ ([LunarG](https://vulkan.lunarg.com/))
- **CMake** 3.20+
- **C++20 Compatible Compiler** (MSVC 2019+, GCC 10+, Clang 11+)
- **vcpkg** (for managing GLFW3 and GLM dependencies)

#### Build

```bash
# Clone repository
git clone https://github.com/66six11/vulkan_study.git
cd vulkan_study

# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Run (Windows)
./bin/Release/vulkan.exe
```

Or use the provided scripts:
```bash
# Windows
build.bat
```

### 📚 Documentation

| Document | Description |
|----------|-------------|
| [PROJECT_PLAN.md](PROJECT_PLAN.md) | Detailed engineering plan, roadmap, and best practices |
| [ProjectStructure.md](ProjectStructure.md) | Project structure, module descriptions, and architecture design |
| [Vulkan项目详解.md](Vulkan项目详解.md) | Vulkan implementation technical details |

### 🔧 Current Version: v0.4.1

**Completed**:
- ✅ Phase 1: Architecture Refactoring & Modularization
- ✅ Renderer abstract interface & VulkanRenderer implementation
- ✅ VulkanDevice, ResourceManager, DescriptorSetManager
- ✅ Vertex data structure definitions (Vertex, VertexInputDescription)

**In Progress**:
- 🔨 Phase 2: Core Rendering Features Extension (vertex buffers, UBO, textures)

> 📘 Full roadmap available in [PROJECT_PLAN.md](PROJECT_PLAN.md)

### 📜 License

MIT License - See [LICENSE](LICENSE) for details
