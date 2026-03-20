# 应用程序入口模块（Apps Module）
# 包含各种应用程序入口（Editor、Demo、Headless 等）
# 依赖层级：Apps -> Application -> Editor -> Rendering -> Vulkan -> Platform -> Core

# Editor 应用程序
set(EDITOR_APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/editor/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/editor/bootstrap/EditorAppBootstrap.cpp
)

set(EDITOR_APP_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/editor/bootstrap/EditorAppBootstrap.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/editor/demo/CubeData.hpp
)

# 创建 Editor 可执行文件
add_executable(vulkan-engine-editor
        ${EDITOR_APP_SOURCES}
        ${EDITOR_APP_HEADERS}
)

add_executable(VulkanEngine::EditorApp ALIAS vulkan-engine-editor)

# 设置包含目录
# PRIVATE: apps/editor 内部实现需要访问完整 include 目录
target_include_directories(vulkan-engine-editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/editor
)

# 链接依赖 - 只链接顶层 Application 模块
target_link_libraries(vulkan-engine-editor PRIVATE
        VulkanEngineApplication
)

# 修复 MSVC 运行时库冲突
fix_msvc_runtime_conflicts(vulkan-engine-editor)

# 设置可执行文件属性
set_target_properties(vulkan-engine-editor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        OUTPUT_NAME "VulkanEngineEditor"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        FOLDER "Apps"
)

# 编译选项
if (MSVC)
    target_compile_options(vulkan-engine-editor PRIVATE
            /W4
            /MP
    )
endif ()

message(STATUS "Created app: vulkan-engine-editor")
message(STATUS "  Sources: ${EDITOR_APP_SOURCES}")
