# 平台抽象层模块（Platform Module）
# 包含窗口、输入、文件系统等

# 创建Platform模块
create_vulkan_module(Platform STATIC)

# 链接依赖
target_link_libraries(VulkanEnginePlatform PUBLIC
        VulkanEngine::GLFW
        VulkanEngineCore
)

# 链接Vulkan
target_include_directories(VulkanEnginePlatform PUBLIC
        ${Vulkan_INCLUDE_DIRS}
)
