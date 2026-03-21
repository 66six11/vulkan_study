cmake_minimum_required(VERSION 3.25)

# =============================================================================
# Engine RHI - Lightweight Vulkan Abstraction Library
# =============================================================================
# 此模块可以:
#   1. 独立构建: cmake -B build -S . -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=../conan_provider.cmake
#   2. 作为子模块: add_subdirectory(engine_rhi) 从父项目
# =============================================================================

# 检测构建模式
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # 独立构建模式
    set(ENGINE_RHI_STANDALONE ON)
    message(STATUS "=== EngineRHI Standalone Build ===")

    # 在独立模式下，尝试加载 Conan provider
    if (DEFINED CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
        if (NOT EXISTS "${CMAKE_PROJECT_TOP_LEVEL_INCLUDES}")
            get_filename_component(CONAN_PROVIDER_NAME "${CMAKE_PROJECT_TOP_LEVEL_INCLUDES}" NAME)
            if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${CONAN_PROVIDER_NAME}")
                set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/${CONAN_PROVIDER_NAME}" CACHE PATH "" FORCE)
            elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${CONAN_PROVIDER_NAME}")
                set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../${CONAN_PROVIDER_NAME}" CACHE PATH "" FORCE)
            endif ()
        endif ()
    else ()
        # 尝试自动找到 conan_provider.cmake
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../conan_provider.cmake")
            set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../conan_provider.cmake"
                    CACHE PATH "" FORCE)
        endif ()
    endif ()
else ()
    # 作为子模块构建
    set(ENGINE_RHI_STANDALONE OFF)
    message(STATUS "=== EngineRHI Submodule Build (parent: ${PROJECT_NAME}) ===")
endif ()

# =============================================================================
# 项目定义
# =============================================================================
project(engine_rhi
        VERSION 1.0.0
        LANGUAGES CXX C
        DESCRIPTION "Lightweight Vulkan RHI abstraction"
)

# C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录 (仅在独立构建时设置)
if (ENGINE_RHI_STANDALONE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

    foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG} CONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
    endforeach ()
endif ()

# =============================================================================
# 源文件
# =============================================================================
set(RHI_HEADERS
        include/engine/rhi/Core.hpp
        include/engine/rhi/Resource.hpp
        include/engine/rhi/Buffer.hpp
        include/engine/rhi/Texture.hpp
        include/engine/rhi/Pipeline.hpp
        include/engine/rhi/CommandBuffer.hpp
        include/engine/rhi/Device.hpp
        include/engine/rhi/SwapChain.hpp
        # Main public API header
)

set(RHI_SOURCES
        src/vulkan/VulkanBuffer.cpp
        src/vulkan/VulkanTexture.cpp
        src/vulkan/VulkanPipeline.cpp
        src/vulkan/VulkanCommandBuffer.cpp
        src/vulkan/VulkanDevice.cpp
        src/vulkan/VulkanSwapChain.cpp
)

# =============================================================================
# 创建库目标
# =============================================================================
add_library(engine_rhi STATIC ${RHI_SOURCES} ${RHI_HEADERS})
add_library(vulkan_engine::rhi ALIAS engine_rhi)

# Include directories
target_include_directories(engine_rhi
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>  # For engine_rhi/vulkan/ compatibility layer
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan
)

# =============================================================================
# 依赖查找
# =============================================================================

# Vulkan (必需)
if (NOT TARGET Vulkan::Vulkan)
    find_package(Vulkan REQUIRED)
endif ()

# Vulkan Memory Allocator
set(VMA_FOUND FALSE)

# 1. 尝试 Conan 包名
if (NOT TARGET GPUOpen::VulkanMemoryAllocator)
    find_package(VulkanMemoryAllocator CONFIG QUIET)
    if (VulkanMemoryAllocator_FOUND)
        set(VMA_FOUND TRUE)
        target_link_libraries(engine_rhi PRIVATE GPUOpen::VulkanMemoryAllocator)
        message(STATUS "  VMA: Conan (GPUOpen::VulkanMemoryAllocator)")
    endif ()
endif ()

# 2. 尝试替代包名
if (NOT VMA_FOUND AND NOT TARGET vulkan-memory-allocator::vulkan-memory-allocator)
    find_package(vulkan-memory-allocator CONFIG QUIET)
    if (vulkan-memory-allocator_FOUND)
        set(VMA_FOUND TRUE)
        target_link_libraries(engine_rhi PRIVATE vulkan-memory-allocator::vulkan-memory-allocator)
        message(STATUS "  VMA: Conan (vulkan-memory-allocator)")
    endif ()
endif ()

# 3. 查找头文件
if (NOT VMA_FOUND)
    find_path(VMA_INCLUDE_DIR
            NAMES vk_mem_alloc.h
            PATHS
            ${Vulkan_INCLUDE_DIRS}
            $ENV{VULKAN_SDK}/Include
    )
    if (VMA_INCLUDE_DIR)
        set(VMA_FOUND TRUE)
        target_include_directories(engine_rhi PUBLIC ${VMA_INCLUDE_DIR})
        message(STATUS "  VMA: Headers at ${VMA_INCLUDE_DIR}")
    elseif (EXISTS "${Vulkan_INCLUDE_DIRS}/vk_mem_alloc.h")
        set(VMA_FOUND TRUE)
        message(STATUS "  VMA: In Vulkan SDK")
    else ()
        message(WARNING "  VMA: Not found!")
    endif ()
endif ()

# 链接 Vulkan
target_link_libraries(engine_rhi PUBLIC Vulkan::Vulkan)

# =============================================================================
# 编译定义
# =============================================================================
target_compile_definitions(engine_rhi
        PUBLIC
        RHI_BACKEND_VULKAN=1
        $<$<BOOL:${VMA_FOUND}>:RHI_VMA_SUPPORTED>
        PRIVATE
        $<$<CONFIG:Debug>:RHI_ENABLE_VALIDATION>
        $<$<CONFIG:Debug>:RHI_ENABLE_DEBUG_MARKERS>
)

# =============================================================================
# 编译器选项
# =============================================================================
if (MSVC)
    target_compile_options(engine_rhi
            PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /EHsc
            /FS      # 允许多个CL.EXE写入同一PDB文件
            /wd4100  # 未引用形参
            /wd4201  # 匿名结构体
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Debug>:/Zi>
    )
    target_link_options(engine_rhi
            PRIVATE
            $<$<CONFIG:Debug>:/DEBUG>
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(engine_rhi
            PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
    )
endif ()

# =============================================================================
# 安装 (仅在独立构建时)
# =============================================================================
if (ENGINE_RHI_STANDALONE)
    include(GNUInstallDirs)

    install(TARGETS engine_rhi
            EXPORT EngineRHITargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT EngineRHITargets
            FILE EngineRHITargets.cmake
            NAMESPACE vulkan_engine::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EngineRHI
    )
endif ()

# =============================================================================
# 配置摘要
# =============================================================================
message(STATUS "")
message(STATUS "EngineRHI Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Backend: Vulkan")
message(STATUS "  Standalone: ${ENGINE_RHI_STANDALONE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Vulkan: ${Vulkan_FOUND}")
message(STATUS "  VMA: ${VMA_FOUND}")
message(STATUS "")
