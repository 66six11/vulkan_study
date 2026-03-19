# cmake/Options.cmake - Project Options

cmake_minimum_required(VERSION 3.25)

#-------------------------------------------------------------------------------
# Project Information
#-------------------------------------------------------------------------------
set(PROJECT_NAME "VulkanEngine" CACHE STRING "Project name")
set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

#-------------------------------------------------------------------------------
# Feature Options
#-------------------------------------------------------------------------------
option(VULKAN_ENGINE_USE_RENDER_GRAPH "Enable Render Graph system" ON)
option(VULKAN_ENGINE_USE_ASYNC_LOADING "Enable async resource loading" ON)
option(VULKAN_ENGINE_USE_HOT_RELOAD "Enable shader hot-reloading" ON)
option(VULKAN_ENGINE_BUILD_TESTS "Build tests" OFF)
option(VULKAN_ENGINE_BUILD_EXAMPLES "Build examples" ON)

#-------------------------------------------------------------------------------
# Debug Options
#-------------------------------------------------------------------------------
option(VULKAN_ENGINE_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(VULKAN_ENGINE_ENABLE_DEBUG_MARKERS "Enable debug markers" ON)
option(VULKAN_ENGINE_ENABLE_PROFILING "Enable profiling" OFF)

#-------------------------------------------------------------------------------
# Performance Options
#-------------------------------------------------------------------------------
option(VULKAN_ENGINE_ENABLE_PCH "Enable precompiled headers" OFF)
option(VULKAN_ENGINE_ENABLE_LTO "Enable link-time optimization" OFF)

#-------------------------------------------------------------------------------
# Developer Options
#-------------------------------------------------------------------------------
option(VULKAN_ENGINE_ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(VULKAN_ENGINE_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
option(VULKAN_ENGINE_ENABLE_CPPCHECK "Enable cppcheck" OFF)

#-------------------------------------------------------------------------------
# Export Options to Parent Scope
#-------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Vulkan Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Use Render Graph: ${VULKAN_ENGINE_USE_RENDER_GRAPH}")
message(STATUS "  Use Async Loading: ${VULKAN_ENGINE_USE_ASYNC_LOADING}")
message(STATUS "  Use Hot Reload: ${VULKAN_ENGINE_USE_HOT_RELOAD}")
message(STATUS "  Build Tests: ${VULKAN_ENGINE_BUILD_TESTS}")
message(STATUS "  Build Examples: ${VULKAN_ENGINE_BUILD_EXAMPLES}")
message(STATUS "  Enable Validation: ${VULKAN_ENGINE_ENABLE_VALIDATION}")
message(STATUS "  Enable PCH: ${VULKAN_ENGINE_ENABLE_PCH}")
message(STATUS "  Enable LTO: ${VULKAN_ENGINE_ENABLE_LTO}")
message(STATUS "")