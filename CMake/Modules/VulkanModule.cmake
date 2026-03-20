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
# 强封口设计：
#   - PUBLIC:  只暴露 include/<module>/ 目录（模块公共接口）
#   - PRIVATE: 只暴露 src/<module>/ 目录（模块私有实现）
#   - 禁止直接暴露整个 include/ 和 src/ 目录
function(create_vulkan_module MODULE_NAME MODULE_TYPE)
    # MODULE_TYPE可以是:
    #   INTERFACE - 头文件模块
    #   STATIC    - 静态库
    #   SHARED    - 动态库

    # 将模块名转换为小写用于目录匹配
    string(TOLOWER ${MODULE_NAME} MODULE_NAME_LOWER)

    # 确定源目录和头目录（严格按模块隔离）
    set(MODULE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/${MODULE_NAME_LOWER}")
    set(MODULE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/${MODULE_NAME_LOWER}")

    # 自动发现源文件
    file(GLOB_RECURSE module_sources
            "${MODULE_SRC_DIR}/*.cpp"
    )

    # 自动发现头文件
    file(GLOB_RECURSE module_headers
            "${MODULE_INCLUDE_DIR}/*.hpp"
            "${MODULE_INCLUDE_DIR}/*.h"
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
                $<BUILD_INTERFACE:${MODULE_INCLUDE_DIR}>
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
        # PUBLIC  - 模块公共接口（其他模块可见）：只暴露本模块的 include/<module>/
        # PRIVATE - 模块内部使用（仅本模块可见）：暴露整个 include/ 和 src/<module>/
        target_include_directories(VulkanEngine${MODULE_NAME}
                PUBLIC
                # 强封口：其他模块只能通过 <module>/xxx.hpp 访问本模块公共接口
                $<BUILD_INTERFACE:${MODULE_INCLUDE_DIR}>
                PRIVATE
                # 内部实现需要访问完整 include/ 目录（兼容现有代码的包含方式）
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                # 本模块的 src 目录
                $<BUILD_INTERFACE:${MODULE_SRC_DIR}>
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
