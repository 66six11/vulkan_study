#include "application/app/Application.hpp"
#include "rendering/render_graph/RenderGraph.hpp"
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
            // Initialize rendering system
            logger::info("Initializing Triangle Application");

            // Set up a simple triangle rendering pipeline
            setup_rendering_pipeline();

            logger::info("Triangle Application initialized successfully");
            return true;
        }

        void on_shutdown() override
        {
            logger::info("Shutting down Triangle Application");
            cleanup_rendering_pipeline();
        }

        void on_update(float delta_time) override
        {
            // Update application logic
            static float rotation = 0.0f;
            rotation += delta_time * 45.0f; // 45 degrees per second

            // Update uniform buffers, camera, etc.
            update_uniforms(rotation);
        }

        void on_render() override
        {
            // Execute the render graph
            if (render_graph_)
            {
                render_graph_->execute();
            }
        }

    private:
        std::unique_ptr<rendering::RenderGraph> render_graph_;

        void setup_rendering_pipeline()
        {
            using namespace rendering;

            // Create a simple render graph for triangle rendering
            render_graph_ = std::make_unique<RenderGraph>();
            auto& builder = render_graph_->builder();

            // Create resources
            auto color_target = builder.create_image({
                                                         .name = "Color Target",
                                                         .type = ResourceDesc::Type::Image,
                                                         .width = config().width,
                                                         .height = config().height,
                                                         .format = ResourceDesc::Format::R8G8B8A8_UNORM
                                                     });

            auto depth_target = builder.create_image({
                                                         .name = "Depth Target",
                                                         .type = ResourceDesc::Type::Image,
                                                         .width = config().width,
                                                         .height = config().height,
                                                         .format = ResourceDesc::Format::D32_SFLOAT
                                                     });

            // Create render passes
            builder.add_pass([color_target, depth_target](CommandBuffer& cmd)
            {
                // Clear targets
                cmd.clear_color(color_target, {0.1f, 0.2f, 0.3f, 1.0f});
                cmd.clear_depth(depth_target, 1.0f);

                // Bind pipeline
                cmd.bind_graphics_pipeline("triangle_pipeline");

                // Draw triangle
                cmd.draw(3, 1, 0, 0);
            });

            // Compile the render graph
            render_graph_->compile();

            logger::info("Rendering pipeline setup complete");
        }

        void cleanup_rendering_pipeline()
        {
            render_graph_.reset();
        }

        void update_uniforms(float rotation)
        {
            // Update uniform buffer data
            // This would typically update camera matrices, object transforms, etc.
        }
};

int main(int argc, char* argv[])
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