#include "bootstrap/EditorAppBootstrap.hpp"
#include "engine/core/utils/Logger.hpp"
#include <iostream>
#include <memory>

using namespace vulkan_engine;

int main(int argc, char* argv[])
{
    logger::info("涓枃鎵撳嵃娴嬭瘯");

    try
    {
        // 瑙ｆ瀽鍛戒护琛岄厤缃?
        auto config = editor::bootstrap::EditorAppConfig::parse(argc, argv);

        // 鍒涘缓 Editor 搴旂敤
        auto app = editor::bootstrap::create_editor_app(config);

        // 鍒濆鍖栧苟杩愯
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