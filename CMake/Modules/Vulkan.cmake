# Vulkan后端模块（Vulkan Module）
# 包含设备管理、资源、管线、同步等

# 创建Vulkan模块
create_vulkan_module(Vulkan STATIC)

# 链接依赖
target_link_libraries(VulkanEngineVulkan PUBLIC
        VulkanEngine::Vulkan
        VulkanEngineRendering
        VulkanEngine::STB
)

# 可选：使用VMA
if (VULKAN_ENGINE_HAS_VMA)
    target_link_libraries(VulkanEngineVulkan PUBLIC VulkanEngine::VMA)
    target_compile_definitions(VulkanEngineVulkan PUBLIC
            VULKAN_ENGINE_USE_VMA=1
    )
endif ()

# 可选：使用SPIRV-Tools
if (VULKAN_ENGINE_HAS_SPIRVTOOLS)
    target_link_libraries(VulkanEngineVulkan PUBLIC VulkanEngine::SPIRVTools)
endif ()

# 根据功能选项添加定义
if (VULKAN_ENGINE_USE_RENDER_GRAPH)
    target_compile_definitions(VulkanEngineVulkan PUBLIC
            VULKAN_ENGINE_USE_RENDER_GRAPH=1
    )
endif ()
