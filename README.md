# Vulkan Study

这是一个 Vulkan API 学习项目，用于研究和实践 Vulkan 图形编程技术。

## 项目特点

- 基于现代 C++20 标准
- 使用 CMake 构建系统  
- 集成 vcpkg 包管理器
- 结构化的 Vulkan 学习代码

## 依赖

- CMake 4.0+
- C++20 兼容编译器 MSVC
- Vulkan SDK 1.4.321.11.4.321.1
- GLFW3
- GLM

## 配置

```cmake
cmake_minimum_required(VERSION 4.0)
project(vulkan)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加 vcpkg 工具链支持，替换为你的实际路径
set(CMAKE_TOOLCHAIN_FILE "C:/Users/C66/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")

set(VULKAN_SDK "C:/VulkanSDK/1.4.321.1")#替换为你的sdk实际路径
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK}/Include")
set(Vulkan_LIBRARY "${VULKAN_SDK}/Lib/vulkan-1.lib")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")


find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm REQUIRED)

add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)
target_include_directories(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)
target_link_libraries(VulkanCppModule
        PUBLIC
        Vulkan::Vulkan
)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS
        "${Vulkan_INCLUDE_DIR}"
        FILES
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

add_executable(${PROJECT_NAME} main.cpp
        HelloTriangleApplication.cpp
        Application.h
)

target_link_libraries(${PROJECT_NAME}
        Vulkan::cppm
        glfw
        glm::glm
)

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
安装vulkan的时候可以将组件全选

请确保已正确安装 Vulkan SDK 并设置了相应的环境变量。

### 硬件是否支持Vulkan SDK？

安装好sdk后可以运行Vulkan Cube.exe检查是否支持

### 构建时遇到依赖问题怎么办？
确保 vcpkg 已正确配置，并且所有依赖包都已安装。

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
