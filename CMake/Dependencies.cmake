# 第三方依赖管理 - 兼容模式

# 查找 Vulkan
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
    add_library(VulkanEngine::Vulkan INTERFACE IMPORTED)
    target_link_libraries(VulkanEngine::Vulkan INTERFACE Vulkan::Vulkan)
    message(STATUS "Vulkan found: ${Vulkan_VERSION}")
else ()
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from https://vulkan.lunarg.com/")
endif ()

# Conan 依赖 - 使用环境变量或手动查找
# 优先使用 CONAN 环境变量，如果没有则尝试标准位置

# GLFW3
if (DEFINED ENV{CONAN_GLFW3_ROOT})
    set(GLFW3_INCLUDE_DIR "$ENV{CONAN_GLFW3_ROOT}/include")
    set(GLFW3_LIBRARY "$ENV{CONAN_GLFW3_ROOT}/lib/glfw3.lib")
else ()
    # 尝试在 Conan 缓存中查找
    find_library(GLFW3_LIBRARY glfw3 glfw
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            "$ENV{USERPROFILE}/.conan2/p/b/glfw*/*/lib"
            PATH_SUFFIXES lib
    )
    find_path(GLFW3_INCLUDE_DIR GLFW/glfw3.h
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            "$ENV{USERPROFILE}/.conan2/p/b/glfw*/*/include"
            PATH_SUFFIXES include
    )
endif ()

if (GLFW3_LIBRARY AND GLFW3_INCLUDE_DIR)
    add_library(VulkanEngine::GLFW INTERFACE IMPORTED)
    target_link_libraries(VulkanEngine::GLFW INTERFACE ${GLFW3_LIBRARY})
    target_include_directories(VulkanEngine::GLFW INTERFACE ${GLFW3_INCLUDE_DIR})
    message(STATUS "GLFW3 found: ${GLFW3_LIBRARY}")
    set(glfw3_FOUND TRUE)
else ()
    message(WARNING "GLFW3 not found via Conan, using stub implementation")
    set(glfw3_FOUND FALSE)
endif ()

# GLM
if (DEFINED ENV{CONAN_GLM_ROOT})
    set(GLM_INCLUDE_DIR "$ENV{CONAN_GLM_ROOT}/include")
else ()
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            "$ENV{USERPROFILE}/.conan2/p/b/glm*/*/include"
            PATH_SUFFIXES include
    )
endif ()

if (GLM_INCLUDE_DIR)
    add_library(VulkanEngine::GLM INTERFACE IMPORTED)
    target_include_directories(VulkanEngine::GLM INTERFACE ${GLM_INCLUDE_DIR})
    message(STATUS "GLM found: ${GLM_INCLUDE_DIR}")
    set(glm_FOUND TRUE)
else ()
    message(WARNING "GLM not found via Conan")
    set(glm_FOUND FALSE)
endif ()

# STB
if (DEFINED ENV{CONAN_STB_ROOT})
    set(STB_INCLUDE_DIR "$ENV{CONAN_STB_ROOT}/include")
else ()
    find_path(STB_INCLUDE_DIR stb_image.h
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            "$ENV{USERPROFILE}/.conan2/p/b/stb*/*/include"
            PATH_SUFFIXES include
    )
endif ()

if (STB_INCLUDE_DIR)
    add_library(VulkanEngine::STB INTERFACE IMPORTED)
    target_include_directories(VulkanEngine::STB INTERFACE ${STB_INCLUDE_DIR})
    message(STATUS "STB found: ${STB_INCLUDE_DIR}")
    set(stb_FOUND TRUE)
else ()
    message(WARNING "STB not found via Conan")
    set(stb_FOUND FALSE)
endif ()

# nlohmann_json for JSON parsing
file(GLOB NLOHMANNJSON_DIRS "$ENV{USERPROFILE}/.conan2/p/nloh*/p/include")

# ImGui for Editor UI
find_package(imgui CONFIG QUIET
        PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/build/generators"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
        NO_DEFAULT_PATH
)
if (imgui_FOUND)
    message(STATUS "ImGui found: ${imgui_VERSION_STRING}")
else ()
    message(WARNING "ImGui not found, Editor UI will be disabled")
endif ()
if (NLOHMANNJSON_DIRS)
    list(GET NLOHMANNJSON_DIRS 0 NLOHMANNJSON_INCLUDE_DIR)
endif ()

if (NLOHMANNJSON_INCLUDE_DIR)
    add_library(VulkanEngine::nlohmann_json INTERFACE IMPORTED)
    target_include_directories(VulkanEngine::nlohmann_json INTERFACE ${NLOHMANNJSON_INCLUDE_DIR})
    message(STATUS "nlohmann_json found: ${NLOHMANNJSON_INCLUDE_DIR}")
    set(nlohmann_json_FOUND TRUE)
else ()
    message(WARNING "nlohmann_json not found via Conan, material JSON loading may fail")
    set(nlohmann_json_FOUND FALSE)
endif ()

# 可选：Taskflow for async loading
if (VULKAN_ENGINE_USE_ASYNC_LOADING)
    find_path(TASKFLOW_INCLUDE_DIR taskflow/taskflow.hpp
            PATHS
            "$ENV{USERPROFILE}/.conan2/p/b/task*/*/include"
            PATH_SUFFIXES include
    )
    if (TASKFLOW_INCLUDE_DIR)
        add_library(VulkanEngine::Taskflow INTERFACE IMPORTED)
        target_include_directories(VulkanEngine::Taskflow INTERFACE ${TASKFLOW_INCLUDE_DIR})
        message(STATUS "Taskflow enabled: ${TASKFLOW_INCLUDE_DIR}")
        set(VULKAN_ENGINE_HAS_TASKFLOW TRUE)
    else ()
        message(WARNING "Taskflow requested but not found, async loading disabled")
        set(VULKAN_ENGINE_HAS_TASKFLOW FALSE)
    endif ()
else ()
    set(VULKAN_ENGINE_HAS_TASKFLOW FALSE)
endif ()

# 可选：Vulkan Memory Allocator
option(VULKAN_ENGINE_USE_VMA "Use Vulkan Memory Allocator" ON)
if (VULKAN_ENGINE_USE_VMA)
    find_package(VulkanMemoryAllocator CONFIG QUIET
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/generators"
            NO_DEFAULT_PATH
    )
    if (VulkanMemoryAllocator_FOUND)
        add_library(VulkanEngine::VMA INTERFACE IMPORTED)
        target_link_libraries(VulkanEngine::VMA INTERFACE GPUOpen::VulkanMemoryAllocator)
        message(STATUS "Vulkan Memory Allocator enabled")
        set(VULKAN_ENGINE_HAS_VMA TRUE)
    else ()
        message(WARNING "VMA requested but not found")
        set(VULKAN_ENGINE_HAS_VMA FALSE)
    endif ()
else ()
    set(VULKAN_ENGINE_HAS_VMA FALSE)
endif ()

# 可选：SPIRV-Tools for shader reflection
if (VULKAN_ENGINE_USE_HOT_RELOAD)
    # 优先查找 Conan 安装的路径
    find_package(SPIRV-Tools CONFIG QUIET
            PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/build/generators"
            "${CMAKE_CURRENT_SOURCE_DIR}/build/build/generators"
            NO_DEFAULT_PATH
    )
    if (NOT SPIRV-Tools_FOUND)
        find_package(SPIRV-Tools CONFIG QUIET)
    endif ()
    if (SPIRV-Tools_FOUND)
        add_library(VulkanEngine::SPIRVTools INTERFACE IMPORTED)
        target_link_libraries(VulkanEngine::SPIRVTools INTERFACE SPIRV-Tools-opt)
        message(STATUS "SPIRV-Tools enabled: ${SPIRV-Tools_DIR}")
        set(VULKAN_ENGINE_HAS_SPIRVTOOLS TRUE)
    else ()
        message(WARNING "SPIRV-Tools not found, shader reflection limited")
        set(VULKAN_ENGINE_HAS_SPIRVTOOLS FALSE)
    endif ()
else ()
    set(VULKAN_ENGINE_HAS_SPIRVTOOLS FALSE)
endif ()

# 导出依赖状态变量
set(VULKAN_ENGINE_DEPENDENCIES_FOUND TRUE)