# Vulkan Study

这是一个 Vulkan API 学习项目，用于研究和实践 Vulkan 图形编程技术。

## 项目特点

- 基于现代 C++20 标准
- 使用 CMake 构建系统  
- 集成 vcpkg 包管理器
- 结构化的 Vulkan 学习代码

## 依赖

- CMake 4.0+
- C++20 兼容编译器
- Vulkan SDK
- GLFW3
- GLM

## 构建

```bash
# 使用 vcpkg 安装依赖
vcpkg install glfw3 glm

# 构建项目
mkdir build
cd build
cmake ..
cmake --build .
```

## Roadmap

- [x] 基础项目结构
- [x] Vulkan 实例创建
- [ ] 设备选择和队列配置
- [ ] 交换链设置
- [ ] 渲染管线创建
- [ ] 绘制三角形

## FAQ

### 如何配置 Vulkan SDK？
请确保已正确安装 Vulkan SDK 并设置了相应的环境变量。

### 构建时遇到依赖问题怎么办？
确保 vcpkg 已正确配置，并且所有依赖包都已安装。

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。