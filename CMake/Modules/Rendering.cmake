# 渲染系统模块（Rendering Module）
# 包含渲染图、资源管理、场景管理等

# 创建Rendering模块
create_vulkan_module(Rendering STATIC)

# 链接依赖
target_link_libraries(VulkanEngineRendering PUBLIC
        VulkanEnginePlatform
        VulkanEngineCore
)

# 根据功能选项添加定义
if (VULKAN_ENGINE_USE_RENDER_GRAPH)
    target_compile_definitions(VulkanEngineRendering PUBLIC
            VULKAN_ENGINE_USE_RENDER_GRAPH=1
    )
endif ()

if (VULKAN_ENGINE_HAS_TASKFLOW)
    target_link_libraries(VulkanEngineRendering PUBLIC VulkanEngine::Taskflow)
    target_compile_definitions(VulkanEngineRendering PUBLIC
            VULKAN_ENGINE_USE_ASYNC_LOADING=1
    )
endif ()
