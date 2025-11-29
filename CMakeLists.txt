cmake_minimum_required(VERSION 3.20)
project(vulkan LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找Vulkan SDK
find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from LunarG.")
endif ()

# 查找GLFW
find_package(glfw3 CONFIG REQUIRED)

if (NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW3 not found. Please install via vcpkg or set up your CMake properly.")
endif ()

# ============================================================================
# 源文件分组 - 按模块组织
# ============================================================================

# Core 模块 - 核心工具和常量
set(CORE_SOURCES
    src/core/constants.cpp
)

set(CORE_HEADERS
    include/core/constants.h
    include/core/TimeStamp.h
)

# Platform 模块 - 平台相关代码
set(PLATFORM_SOURCES
    src/platform/main.cpp
    src/platform/VulkanApp.cpp
)

set(PLATFORM_HEADERS
    include/platform/Application.h
    include/platform/Platform.h
)

# Renderer 模块 - 渲染器抽象接口
set(RENDERER_HEADERS
    include/renderer/Renderer.h
)

# Vulkan Backend 模块 - Vulkan 后端实现
set(VULKAN_BACKEND_SOURCES
    src/vulkan_backend/VulkanDevice.cpp
    src/vulkan_backend/VulkanRenderer.cpp
    src/vulkan_backend/ResourceManager.cpp
    src/vulkan_backend/DescriptorSetManager.cpp
    src/vulkan_backend/SwapchainResources.cpp
    src/vulkan_backend/vulkan_init.cpp
    src/vulkan_backend/swapchain_management.cpp
    src/vulkan_backend/Rendering.cpp
    src/vulkan_backend/command_buffer_sync.cpp
    src/vulkan_backend/utils.cpp
)

set(VULKAN_BACKEND_HEADERS
    include/vulkan_backend/VulkanDevice.h
    include/vulkan_backend/VulkanRenderer.h
    include/vulkan_backend/ResourceManager.h
    include/vulkan_backend/DescriptorSetManager.h
    include/vulkan_backend/SwapchainResources.h
    include/vulkan_backend/vulkan_init.h
    include/vulkan_backend/swapchain_management.h
    include/vulkan_backend/Rendering.h
    include/vulkan_backend/command_buffer_sync.h
    include/vulkan_backend/utils.h
)

# ============================================================================
# 添加可执行文件
# ============================================================================

add_executable(vulkan
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${PLATFORM_SOURCES}
    ${PLATFORM_HEADERS}
    ${RENDERER_HEADERS}
    ${VULKAN_BACKEND_SOURCES}
    ${VULKAN_BACKEND_HEADERS}
)

# IDE 源文件分组 (Visual Studio / Xcode)
source_group("Core\\Source Files" FILES ${CORE_SOURCES})
source_group("Core\\Header Files" FILES ${CORE_HEADERS})
source_group("Platform\\Source Files" FILES ${PLATFORM_SOURCES})
source_group("Platform\\Header Files" FILES ${PLATFORM_HEADERS})
source_group("Renderer\\Header Files" FILES ${RENDERER_HEADERS})
source_group("Vulkan Backend\\Source Files" FILES ${VULKAN_BACKEND_SOURCES})
source_group("Vulkan Backend\\Header Files" FILES ${VULKAN_BACKEND_HEADERS})

# ============================================================================
# 链接库
# ============================================================================

target_link_libraries(vulkan PRIVATE
    Vulkan::Vulkan
    glfw
)

# ============================================================================
# 包含目录
# ============================================================================

target_include_directories(vulkan PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# 如果找到了GLFW包含目录
if (GLFW_INCLUDE_DIRS)
    target_include_directories(vulkan PRIVATE ${GLFW_INCLUDE_DIRS})
endif ()

# ============================================================================
# 平台特定设置
# ============================================================================

# 设置Visual Studio调试工作目录
set_target_properties(vulkan PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ============================================================================
# 着色器编译
# ============================================================================

find_program(GLSLC glslc)

if (NOT GLSLC)
    message(WARNING "glslc not found. Shaders will not be compiled automatically.")
else ()
    # 编译着色器
    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
    set(COMPILED_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)

    file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})

    # 顶点着色器编译规则
    add_custom_command(
        OUTPUT ${COMPILED_SHADER_DIR}/shader.vert.spv
        COMMAND ${GLSLC} -o ${COMPILED_SHADER_DIR}/shader.vert.spv ${SHADER_DIR}/shader.vert
        DEPENDS ${SHADER_DIR}/shader.vert
        COMMENT "Compiling vertex shader"
    )

    # 片段着色器编译规则
    add_custom_command(
        OUTPUT ${COMPILED_SHADER_DIR}/shader.frag.spv
        COMMAND ${GLSLC} -o ${COMPILED_SHADER_DIR}/shader.frag.spv ${SHADER_DIR}/shader.frag
        DEPENDS ${SHADER_DIR}/shader.frag
        COMMENT "Compiling fragment shader"
    )

    # 创建着色器目标
    add_custom_target(compile_shaders
        DEPENDS ${COMPILED_SHADER_DIR}/shader.vert.spv ${COMPILED_SHADER_DIR}/shader.frag.spv
    )
    add_dependencies(vulkan compile_shaders)

    # 设置输出目录为运行时目录
    add_custom_command(TARGET vulkan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${COMPILED_SHADER_DIR} $<TARGET_FILE_DIR:vulkan>/shaders
    )
endif ()