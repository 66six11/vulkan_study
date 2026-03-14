# 应用程序模块（Application Module）
# 包含主应用程序和配置管理

# 创建Application模块
create_vulkan_module(Application STATIC)

# 链接依赖
target_link_libraries(VulkanEngineApplication PUBLIC
        VulkanEngineVulkan
        VulkanEngineEditor
)

# 创建主可执行文件
add_executable(vulkan-engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(vulkan-engine PRIVATE VulkanEngineApplication)

# 设置可执行文件属性
set_target_properties(vulkan-engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        OUTPUT_NAME "VulkanEngine"
)

# 控制台应用程序（使用main函数作为入口点）
# 注意：如果需要纯GUI应用程序（无控制台窗口），可以设置为WIN32_EXECUTABLE

# 如果使用预编译头，为可执行文件配置
if (VULKAN_ENGINE_ENABLE_PCH)
    # 需要额外配置PCH
    # target_precompile_headers(...)
    message(STATUS "Precompiled headers enabled (configuration required)")
endif ()
