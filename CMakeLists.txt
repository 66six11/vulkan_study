cmake_minimum_required(VERSION 3.25)
project(VulkanEngine VERSION 2.0.0 LANGUAGES CXX)

# Set C++ standard and properties
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(VULKAN_ENGINE_USE_RENDER_GRAPH "Enable Render Graph system" ON)
option(VULKAN_ENGINE_USE_ASYNC_LOADING "Enable async resource loading" ON)
option(VULKAN_ENGINE_USE_HOT_RELOAD "Enable shader hot-reloading" ON)
option(VULKAN_ENGINE_BUILD_TESTS "Build tests" OFF)
option(VULKAN_ENGINE_BUILD_EXAMPLES "Build examples" ON)

# Configure compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif ()
elseif (MSVC)
    add_compile_options(/W4 /permissive-)
endif ()

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find or fetch dependencies
# Note: Dependencies should be provided by Conan 2.0
# Run: conan install . --build=missing --output-folder=build

# Vulkan SDK (expected to be installed system-wide)
find_package(Vulkan REQUIRED)

# GLFW3 (provided by Conan)
find_package(glfw3 CONFIG REQUIRED)

# GLM (provided by Conan)
find_package(glm CONFIG REQUIRED)

# STB (provided by Conan, header-only)
find_package(stb CONFIG REQUIRED)

# Optional: Taskflow for async loading (provided by Conan if enabled)
if (VULKAN_ENGINE_USE_ASYNC_LOADING)
    find_package(taskflow CONFIG QUIET)
    if (taskflow_FOUND)
        target_compile_definitions(VulkanEngineRendering PUBLIC VULKAN_ENGINE_USE_ASYNC_LOADING=1)
    endif ()
endif ()

# Core library (header-only for now)
add_library(VulkanEngineCore INTERFACE)
target_link_libraries(VulkanEngineCore INTERFACE glm)
target_include_directories(VulkanEngineCore INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform abstraction layer
add_library(VulkanEnginePlatform STATIC
        src/platform/windowing/Window.cpp
        src/platform/input/InputManager.cpp
        src/platform/filesystem/FileSystem.cpp
)
target_link_libraries(VulkanEnginePlatform PUBLIC
        glfw
        VulkanEngineCore
)
target_include_directories(VulkanEnginePlatform PUBLIC
        ${Vulkan_INCLUDE_DIRS}
)

# Rendering system
if (VULKAN_ENGINE_USE_RENDER_GRAPH)
    add_library(VulkanEngineRendering STATIC
            src/rendering/render_graph/RenderGraph.cpp
            src/rendering/resources/ResourceManager.cpp
            src/rendering/shaders/ShaderManager.cpp
            src/rendering/scene/Scene.cpp
    )
    target_compile_definitions(VulkanEngineRendering PUBLIC VULKAN_ENGINE_USE_RENDER_GRAPH=1)
else ()
    add_library(VulkanEngineRendering STATIC
            src/rendering/resources/ResourceManager.cpp
            src/rendering/shaders/ShaderManager.cpp
            src/rendering/scene/Scene.cpp
    )
endif ()
target_link_libraries(VulkanEngineRendering PUBLIC VulkanEnginePlatform)

# Vulkan backend
add_library(VulkanEngineVulkan STATIC
        src/vulkan/device/Device.cpp
        src/vulkan/resources/Buffer.cpp
        src/vulkan/resources/Image.cpp
        src/vulkan/pipelines/Pipeline.cpp
        src/vulkan/sync/Synchronization.cpp
)
target_link_libraries(VulkanEngineVulkan PUBLIC
        Vulkan::Vulkan
        VulkanEngineRendering
)

# Main application library
add_library(VulkanEngine STATIC
        src/application/app/Application.cpp
        src/application/config/Config.cpp
)
target_link_libraries(VulkanEngine PUBLIC VulkanEngineVulkan)

# Main executable
add_executable(vulkan-engine src/main.cpp)
target_link_libraries(vulkan-engine VulkanEngine)

# Set output directory
set_target_properties(vulkan-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "Vulkan Engine Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Use Render Graph: ${VULKAN_ENGINE_USE_RENDER_GRAPH}")
message(STATUS "  GLFW3 Found: ${glfw3_FOUND}")
message(STATUS "  GLM Found: ${glm_FOUND}")
message(STATUS "  Vulkan Found: ${Vulkan_FOUND}")
message(STATUS "")