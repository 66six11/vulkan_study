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

# Find dependencies using modern CMake
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Core library
add_library(VulkanEngineCore
        src/core/math/Vector.hpp
        src/core/math/Matrix.hpp
        src/core/math/Quaternion.hpp
        src/core/memory/MemoryPool.hpp
        src/core/memory/Allocator.hpp
        src/core/utils/Logger.hpp
        src/core/utils/Profiler.hpp
)

target_link_libraries(VulkanEngineCore PUBLIC
        glm::glm
)

# Platform abstraction layer
add_library(VulkanEnginePlatform
        src/platform/windowing/Window.hpp
        src/platform/windowing/Window.cpp
        src/platform/input/InputManager.hpp
        src/platform/input/InputManager.cpp
        src/platform/filesystem/FileSystem.hpp
        src/platform/filesystem/FileSystem.cpp
)

target_link_libraries(VulkanEnginePlatform PUBLIC
        glfw
        VulkanEngineCore
)

# Rendering system
if (VULKAN_ENGINE_USE_RENDER_GRAPH)
    add_library(VulkanEngineRendering
            src/rendering/render_graph/RenderGraph.hpp
            src/rendering/render_graph/RenderGraph.cpp
            src/rendering/render_graph/Pass.hpp
            src/rendering/render_graph/Resource.hpp
            src/rendering/resources/ResourceManager.hpp
            src/rendering/resources/ResourceManager.cpp
            src/rendering/shaders/ShaderManager.hpp
            src/rendering/shaders/ShaderManager.cpp
            src/rendering/scene/Scene.hpp
            src/rendering/scene/Scene.cpp
    )
else ()
    add_library(VulkanEngineRendering
            src/rendering/resources/ResourceManager.hpp
            src/rendering/resources/ResourceManager.cpp
            src/rendering/shaders/ShaderManager.hpp
            src/rendering/shaders/ShaderManager.cpp
            src/rendering/scene/Scene.hpp
            src/rendering/scene/Scene.cpp
    )
endif ()

target_link_libraries(VulkanEngineRendering PUBLIC
        VulkanEnginePlatform
)

# Vulkan backend
add_library(VulkanEngineVulkan
        src/vulkan/device/Device.hpp
        src/vulkan/device/Device.cpp
        src/vulkan/resources/Buffer.hpp
        src/vulkan/resources/Buffer.cpp
        src/vulkan/resources/Image.hpp
        src/vulkan/resources/Image.cpp
        src/vulkan/pipelines/Pipeline.hpp
        src/vulkan/pipelines/Pipeline.cpp
        src/vulkan/sync/Synchronization.hpp
        src/vulkan/sync/Synchronization.cpp
)

target_link_libraries(VulkanEngineVulkan PUBLIC
        Vulkan::Vulkan
        VulkanEngineRendering
)

# Main application library
add_library(VulkanEngine
        src/application/app/Application.hpp
        src/application/app/Application.cpp
        src/application/config/Config.hpp
        src/application/config/Config.cpp
)

target_link_libraries(VulkanEngine PUBLIC
        VulkanEngineVulkan
)

# Main executable
add_executable(vulkan-engine
        src/main.cpp
)

target_link_libraries(vulkan-engine
        VulkanEngine
)

# Installation
install(TARGETS vulkan-engine DESTINATION bin)
install(TARGETS VulkanEngine DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Package configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VulkanEngineConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VulkanEngineConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VulkanEngineConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VulkanEngine
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VulkanEngineConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VulkanEngineConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VulkanEngine
)

# Testing
if (VULKAN_ENGINE_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 REQUIRED)

    add_executable(vulkan-engine-tests
            tests/core_tests.cpp
            tests/math_tests.cpp
            tests/rendering_tests.cpp
    )

    target_link_libraries(vulkan-engine-tests
            VulkanEngine
            Catch2::Catch2WithMain
    )

    add_test(NAME vulkan-engine-tests COMMAND vulkan-engine-tests)
endif ()

# Examples
if (VULKAN_ENGINE_BUILD_EXAMPLES)
    add_executable(example-triangle
            examples/triangle/main.cpp
    )

    target_link_libraries(example-triangle
            VulkanEngine
    )
endif ()