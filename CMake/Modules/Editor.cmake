# 编辑器模块（Editor Module）
# 包含 ImGui UI、场景视口等编辑器功能
# 独立于 Rendering 模块，负责编辑界面

cmake_minimum_required(VERSION 3.25)

# 收集源文件
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/*.cpp
)

file(GLOB_RECURSE EDITOR_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/editor/*.hpp
)

# ImGui backend implementation files (from Conan package)
if (imgui_FOUND)
    list(APPEND EDITOR_SOURCES
            ${imgui_PACKAGE_FOLDER_RELEASE}/res/bindings/imgui_impl_vulkan.cpp
            ${imgui_PACKAGE_FOLDER_RELEASE}/res/bindings/imgui_impl_glfw.cpp
    )
endif ()

# 创建 Editor 模块
add_library(VulkanEngineEditor STATIC
        ${EDITOR_SOURCES}
        ${EDITOR_HEADERS}
)

add_library(VulkanEngine::Editor ALIAS VulkanEngineEditor)

# 强封口配置：
# PUBLIC  - 其他模块只能通过 editor/xxx.hpp 访问本模块公共接口
# PRIVATE - 内部实现需要访问完整 include/ 目录（兼容现有代码的包含方式）
target_include_directories(VulkanEngineEditor
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/editor>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/editor>
)

# 链接依赖 - Editor 只依赖上层 Rendering 模块
# 注意：不直接依赖 Vulkan 模块，通过 Rendering 间接使用
target_link_libraries(VulkanEngineEditor PUBLIC
        VulkanEngineRendering
        VulkanEnginePlatform
        VulkanEngineCore
)

# ImGui library linking
if (imgui_FOUND)
    # Always link the imgui library directly (Conan target may not work correctly)
    target_link_libraries(VulkanEngineEditor PUBLIC
            ${imgui_PACKAGE_FOLDER_RELEASE}/lib/imgui.lib
    )
    # Also add system libs for Windows
    if (WIN32)
        target_link_libraries(VulkanEngineEditor PUBLIC imm32)
    endif ()
    target_include_directories(VulkanEngineEditor PUBLIC
            ${imgui_INCLUDE_DIRS_RELEASE}
            ${imgui_PACKAGE_FOLDER_RELEASE}/res/bindings
    )
    target_compile_definitions(VulkanEngineEditor PUBLIC
            VULKAN_ENGINE_USE_IMGUI=1
    )
endif ()

# 设置目标属性
set_target_properties(VulkanEngineEditor PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        FOLDER "Engine/Modules"
)

# 编译选项
if (MSVC)
    target_compile_options(VulkanEngineEditor PRIVATE
            /W4
            /MP
    )
endif ()

message(STATUS "Created module: VulkanEngineEditor")
message(STATUS "  Sources: ${EDITOR_SOURCES}")
message(STATUS "  Headers: ${EDITOR_HEADERS}")