# 核心模块（Core Module）
# 包含基础工具、数学库、内存管理等

# 创建Core模块（头文件模块）
create_vulkan_module(Core INTERFACE)

# 链接GLM数学库
target_link_libraries(VulkanEngineCore INTERFACE VulkanEngine::GLM)

# 根据功能选项添加定义
if (VULKAN_ENGINE_ENABLE_VALIDATION)
    target_compile_definitions(VulkanEngineCore INTERFACE
            VULKAN_ENGINE_ENABLE_VALIDATION=1
    )
endif ()

if (VULKAN_ENGINE_ENABLE_DEBUG_MARKERS)
    target_compile_definitions(VulkanEngineCore INTERFACE
            VULKAN_ENGINE_ENABLE_DEBUG_MARKERS=1
    )
endif ()

if (VULKAN_ENGINE_ENABLE_PROFILING)
    target_compile_definitions(VulkanEngineCore INTERFACE
            VULKAN_ENGINE_ENABLE_PROFILING=1
    )
endif ()
