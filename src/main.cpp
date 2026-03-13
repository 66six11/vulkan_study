#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"

#include <iostream>
#include <memory>

using namespace vulkan_engine;

class TriangleApplication : public application::ApplicationBase
{
    public:
        explicit TriangleApplication(const application::ApplicationConfig& config)
            : application::ApplicationBase(config)
        {
        }

        bool on_initialize() override
        {
            logger::info("Initializing Triangle Application");
            logger::info("Window created: " + std::to_string(config().width) + "x" + std::to_string(config().height));
            return true;
        }

        void on_shutdown() override
        {
            logger::info("Shutting down Triangle Application");
        }

        void on_update(float delta_time) override
        {
            static float total_time = 0.0f;
            total_time += delta_time;

            // Print FPS every second
            static int frame_count = 0;
            frame_count++;
            if (total_time >= 1.0f)
            {
                std::cout << "FPS: " << frame_count << std::endl;
                frame_count = 0;
                total_time -= 1.0f;
            }
        }

        void on_render() override
        {
            // Rendering will be implemented here
        }
};

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
        // Configure the application
        application::ApplicationConfig config{
            .title = "Modern Vulkan Engine - Triangle Demo",
            .width = 1280,
            .height = 720,
            .vsync = true,
            .enable_validation = true,
            .enable_profiling = true,
            .use_render_graph = true,
            .use_async_loading = true,
            .use_hot_reload = true
        };

        // Create and run the application
        auto app = std::make_unique<TriangleApplication>(config);

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