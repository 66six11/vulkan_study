cmake_minimum_required(VERSION 3.25)

# 加载 Conan toolchain (必须在 project() 之前)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators/conan_toolchain.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators/conan_toolchain.cmake")
endif ()

project(VulkanEngine VERSION 2.0.0 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 强制 MSVC 使用动态运行时库 (/MD /MDd) 以匹配 Conan 依赖
# 这必须在 Conan toolchain 加载后设置，但要在任何目标创建之前
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    message(STATUS "强制 MSVC 运行时库: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif ()

# 设置 Conan 包路径 (generators 目录包含包配置)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators")
endif ()

# 加载项目选项
include(cmake/Options.cmake)

# 加载编译器标志
include(cmake/CompilerFlags.cmake)

# 查找依赖
include(cmake/Dependencies.cmake)

# 加载模块构建系统
include(cmake/Modules/VulkanModule.cmake)

# 加载核心模块
include(cmake/Modules/Core.cmake)

# 加载平台模块
include(cmake/Modules/Platform.cmake)

# 加载渲染模块
include(cmake/Modules/Rendering.cmake)

# 加载 Vulkan 模块
include(cmake/Modules/Vulkan.cmake)

# 加载编辑器模块（依赖 Rendering）
include(cmake/Modules/Editor.cmake)

# 加载应用程序模块
include(cmake/Modules/Application.cmake)

# 打印配置摘要
include(cmake/Utils.cmake)
print_configuration_summary()

# 可选：创建测试目标
if (VULKAN_ENGINE_BUILD_TESTS)
    enable_testing()
    message(STATUS "Tests enabled (not yet implemented)")
endif ()

# 可选：创建示例目标
if (VULKAN_ENGINE_BUILD_EXAMPLES)
    message(STATUS "Examples enabled (not yet implemented)")
endif ()

# 复制着色器文件到输出目录
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.spv")
if (SHADER_FILES)
    # 着色器输出目录 - 为每个配置复制 (Debug/Release)
    set(SHADER_OUTPUT_DIR_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug/shaders")
    set(SHADER_OUTPUT_DIR_RELEASE "${CMAKE_BINARY_DIR}/bin/Release/shaders")

    add_custom_target(CopyShaders ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR_DEBUG}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR_RELEASE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_FILES} "${SHADER_OUTPUT_DIR_DEBUG}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_FILES} "${SHADER_OUTPUT_DIR_RELEASE}/"
            COMMENT "Copying shader files to Debug/Release directories"
    )
    list(LENGTH SHADER_FILES SHADER_COUNT)
    message(STATUS "Found ${SHADER_COUNT} shader file(s)")
else ()
    message(WARNING "No compiled shader files (.spv) found in shaders/ directory. Run shaders/compile_slang.bat to generate them.")
endif ()