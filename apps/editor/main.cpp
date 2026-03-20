#include "bootstrap/EditorAppBootstrap.hpp"
#include "core/utils/Logger.hpp"
#include <iostream>
#include <memory>

using namespace vulkan_engine;

int main(int argc, char* argv[])
{
    logger::info("中文打印测试");

    try
    {
        // 解析命令行配置
        auto config = editor::bootstrap::EditorAppConfig::parse(argc, argv);

        // 创建 Editor 应用
        auto app = editor::bootstrap::create_editor_app(config);

        // 初始化并运行
        if (app->initialize())
        {
            app->run();
        }

        app->shutdown();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}