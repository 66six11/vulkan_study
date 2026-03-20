#pragma once

// 需要包含 Application 头文件来获取基类定义
// 注意：这是 apps/editor 的特权，它作为应用程序入口可以访问引擎各层
#include "application/app/Application.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace vulkan_engine::rendering
{
    class ComposedRenderer;
}

namespace editor::bootstrap
{
    // 使用引擎命名空间
    using namespace vulkan_engine;

    // Forward declarations
    class EditorApplication;

    // Editor App 配置结构
    struct EditorAppConfig
    {
        std::string title             = "Vulkan Engine - Editor";
        uint32_t    width             = 1600;
        uint32_t    height            = 900;
        bool        vsync             = true;
        bool        enable_validation = true;
        bool        enable_profiling  = true;

        // 从命令行参数解析配置
        static EditorAppConfig parse(int argc, char* argv[]);
    };

    // Editor Application 工厂函数
    std::unique_ptr<EditorApplication> create_editor_app(const EditorAppConfig& config);

    // Editor Application 类声明
    class EditorApplication : public vulkan_engine::application::ApplicationBase
    {
        public:
            explicit EditorApplication(const vulkan_engine::application::ApplicationConfig& config);
            ~EditorApplication() override = default;

            // 禁用拷贝
            EditorApplication(const EditorApplication&)            = delete;
            EditorApplication& operator=(const EditorApplication&) = delete;

            // 允许移动
            EditorApplication(EditorApplication&&) noexcept            = default;
            EditorApplication& operator=(EditorApplication&&) noexcept = default;

        protected:
            bool on_initialize() override;
            void on_shutdown() override;
            void on_update(float delta_time) override;
            void on_render() override;
            void on_window_resize(const vulkan_engine::application::WindowResizeEvent& event) override;

        private:
            class Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace editor::bootstrap