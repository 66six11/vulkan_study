# 应用程序模块（Application Module）
# 包含主应用程序和配置管理
# 依赖层级：Application -> Editor -> Rendering -> Vulkan -> Platform -> Core

# 创建Application模块
create_vulkan_module(Application STATIC)

# 链接依赖 - Application 只依赖 Editor 模块
# 注意：不直接依赖 Vulkan 模块，通过 Editor -> Rendering -> Vulkan 间接使用
target_link_libraries(VulkanEngineApplication PUBLIC
        VulkanEngineEditor
        VulkanEnginePlatform
        VulkanEngineCore
)

# 注意：可执行文件入口已迁移到 apps/editor/
# 旧的 src/main.cpp 保留用于参考，但不再作为构建目标
# 新的入口点在 apps/editor/main.cpp，由 Apps.cmake 创建
