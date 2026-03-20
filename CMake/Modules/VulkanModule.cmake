# 模块化CMake构建系统
# 自动创建和配置Vulkan引擎各个模块

# 定义模块列表（按依赖顺序 - 从底层到上层）
set(VULKAN_ENGINE_MODULES
        Core
        Platform
        Vulkan
        Rendering
        Editor
        Application
)

# 创建Vulkan模块的通用函数
# 支持新的 engine/ 目录结构：
#   - engine/<module>/include/engine/<module>/  (公共头文件)
#   - engine/<module>/src/                      (私有实现)
# 强封口设计：
#   - PUBLIC:  只暴露 engine/<module>/include/engine/<module>/ 目录
#   - PRIVATE: 暴露 engine/<module>/src/ 目录和内部包含路径
function(create_vulkan_module MODULE_NAME MODULE_TYPE)
    # MODULE_TYPE可以是:
    #   INTERFACE - 头文件模块
    #   STATIC    - 静态库
    #   SHARED    - 动态库

    # 将模块名转换为小写用于目录匹配
    string(TOLOWER ${MODULE_NAME} MODULE_NAME_LOWER)

    # 确定新的 engine/ 目录结构路径
    set(ENGINE_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/${MODULE_NAME_LOWER}")
    set(ENGINE_SRC_DIR "${ENGINE_MODULE_DIR}/src")
    set(ENGINE_INCLUDE_DIR "${ENGINE_MODULE_DIR}/include")

    # Vulkan模块特殊处理：现在位于 engine/rhi/vulkan/
    if (MODULE_NAME STREQUAL "Vulkan")
        set(ENGINE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/rhi/src/vulkan")
        set(ENGINE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/rhi/include")
    endif ()

    # 自动发现源文件（新结构）
    file(GLOB_RECURSE module_sources
            "${ENGINE_SRC_DIR}/*.cpp"
    )

    # 自动发现头文件（新结构）
    file(GLOB_RECURSE module_headers
            "${ENGINE_INCLUDE_DIR}/*.hpp"
            "${ENGINE_INCLUDE_DIR}/*.h"
    )

    # 打印发现的文件信息（仅在有文件时）
    if (NOT "${module_sources}" STREQUAL "")
        list(LENGTH module_sources source_count)
        message(STATUS "  Found ${source_count} source file(s) in ${MODULE_NAME_LOWER}")
    endif ()

    if (NOT "${module_headers}" STREQUAL "")
        list(LENGTH module_headers header_count)
        message(STATUS "  Found ${header_count} header file(s) in ${MODULE_NAME_LOWER}")
    endif ()

    # 创建库目标
    if (MODULE_TYPE STREQUAL "INTERFACE")
        add_library(VulkanEngine${MODULE_NAME} INTERFACE)
        target_sources(VulkanEngine${MODULE_NAME} INTERFACE ${module_headers})

        # INTERFACE 模块：只暴露公共头文件目录
        target_include_directories(VulkanEngine${MODULE_NAME} INTERFACE
                $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}>
        )
    else ()
        if ("${module_sources}" STREQUAL "")
            message(WARNING "  No source files found for ${MODULE_NAME}, creating empty library")
        endif ()
        add_library(VulkanEngine${MODULE_NAME} ${MODULE_TYPE}
                ${module_sources}
                ${module_headers}
        )

        # 强封口配置：
        # PUBLIC  - 模块公共接口（其他模块可见）：暴露 engine/<module>/include/
        # PRIVATE - 模块内部使用（仅本模块可见）：暴露 engine/<module>/src/
        target_include_directories(VulkanEngine${MODULE_NAME}
                PUBLIC
                # 公共接口：其他模块可以通过 <engine/module>/xxx.hpp 访问
                $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}>
                PRIVATE
                # 内部实现使用本模块的 src 目录
                $<BUILD_INTERFACE:${ENGINE_SRC_DIR}>
        )
    endif ()

    # 应用编译器标志
    apply_compiler_flags(VulkanEngine${MODULE_NAME})

    # 启用静态分析
    enable_static_analysis(VulkanEngine${MODULE_NAME})

    # 设置输出目录
    if (NOT MODULE_TYPE STREQUAL "INTERFACE")
        set_target_properties(VulkanEngine${MODULE_NAME} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif ()

    message(STATUS "Created module: VulkanEngine${MODULE_NAME} (${MODULE_TYPE})")
endfunction()
