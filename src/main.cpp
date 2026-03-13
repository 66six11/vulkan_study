#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/pipelines/ShaderModule.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <filesystem>

using namespace vulkan_engine;

// Simple vertex structure for triangle
struct Vertex
{
    float position[2];
    float color[3];
};

// Triangle vertices
const std::vector<Vertex> triangle_vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    // Top - Red
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    // Bottom Right - Green
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} // Bottom Left - Blue
};

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

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain)
            {
                logger::error("Device or swap chain not initialized");
                return false;
            }

            // Create frame sync manager (2 frames in flight)
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2);

            // Create framebuffer pool for swap chain images
            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device);
            create_framebuffers();

            // Create command pool and buffers
            cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device,
                                                                    0,
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            cmd_buffers_ = cmd_pool_->allocate(swap_chain->image_count());

            // Create vertex buffer
            create_vertex_buffer();

            // Create graphics pipeline
            create_pipeline();

            // Register swap chain resize callback
            swap_chain->on_recreate([this](uint32_t width, uint32_t height)
            {
                recreate_resources(width, height);
            });

            logger::info("Triangle Application initialized successfully");
            return true;
        }

        void on_shutdown() override
        {
            logger::info("Shutting down Triangle Application");

            // Wait for device idle before cleanup
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Cleanup in reverse order
            pipeline_.reset();
            vertex_buffer_.reset();
            cmd_buffers_.clear();
            cmd_pool_.reset();
            framebuffer_pool_.reset();
            frame_sync_.reset();
        }

        void on_update(float delta_time) override
        {
            static float total_time = 0.0f;
            total_time += delta_time;

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
            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain || !pipeline_)
            {
                return;
            }

            // Wait for current frame's fence
            frame_sync_->wait_and_reset_current_fence();

            // Acquire next image
            uint32_t    image_index     = 0;
            VkSemaphore image_available = frame_sync_->get_current_image_available_semaphore().handle();
            if (!swap_chain->acquire_next_image(image_available, VK_NULL_HANDLE, image_index))
            {
                // Swap chain needs recreation
                return;
            }

            // Get command buffer for this image
            auto& cmd = cmd_buffers_[image_index];
            cmd.reset();
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Begin render pass
            VkClearValue clear_color{};
            clear_color.color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Black background

            auto* framebuffer = framebuffer_pool_->get_framebuffer(image_index);
            cmd.begin_render_pass(
                                  swap_chain->default_render_pass(),
                                  framebuffer->handle(),
                                  swap_chain->width(),
                                  swap_chain->height(),
                                  clear_color);

            // Bind pipeline
            cmd.bind_pipeline(pipeline_->handle());

            // Set viewport and scissor
            cmd.set_viewport(0.0f, 0.0f, static_cast<float>(swap_chain->width()), static_cast<float>(swap_chain->height()));
            cmd.set_scissor(0, 0, swap_chain->width(), swap_chain->height());

            // Bind vertex buffer and draw
            if (vertex_buffer_)
            {
                cmd.bind_vertex_buffer(vertex_buffer_->handle());
                cmd.draw(3, 1, 0, 0); // 3 vertices, 1 instance
            }

            cmd.end_render_pass();
            cmd.end();

            // Submit
            VkSemaphore          wait_semaphores[]   = {image_available};
            VkSemaphore          signal_semaphores[] = {frame_sync_->get_current_render_finished_semaphore().handle()};
            VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            VkCommandBuffer cmd_handle = cmd.handle();
            VkSubmitInfo    submit_info{};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 1;
            submit_info.pWaitSemaphores      = wait_semaphores;
            submit_info.pWaitDstStageMask    = wait_stages;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &cmd_handle;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = signal_semaphores;

            VkFence fence = frame_sync_->get_current_fence().handle();
            vkQueueSubmit(device->graphics_queue(), 1, &submit_info, fence);

            // Present
            VkSemaphore present_semaphore = frame_sync_->get_current_render_finished_semaphore().handle();
            swap_chain->present(device->graphics_queue(), image_index, present_semaphore);

            // Next frame
            frame_sync_->next_frame();
        }

    private:
        void create_framebuffers()
        {
            auto                     swap_chain = this->swap_chain();
            std::vector<VkImageView> image_views;
            image_views.reserve(swap_chain->image_count());
            for (uint32_t i = 0; i < swap_chain->image_count(); ++i)
            {
                image_views.push_back(swap_chain->get_image(i).view);
            }

            framebuffer_pool_->create_for_swap_chain(
                                                     swap_chain->default_render_pass(),
                                                     image_views,
                                                     swap_chain->width(),
                                                     swap_chain->height());
        }

        void create_vertex_buffer()
        {
            auto         device      = device_manager();
            VkDeviceSize buffer_size = sizeof(triangle_vertices[0]) * triangle_vertices.size();

            vertex_buffer_ = std::make_unique<vulkan::Buffer>(
                                                              device,
                                                              buffer_size,
                                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void* data = vertex_buffer_->map();
            memcpy(data, triangle_vertices.data(), static_cast<size_t>(buffer_size));
            vertex_buffer_->unmap();
        }

        void create_pipeline()
        {
            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            // Check render pass
            if (swap_chain->default_render_pass() == VK_NULL_HANDLE)
            {
                logger::error("SwapChain default render pass is null!");
                throw std::runtime_error("Invalid render pass");
            }

            vulkan::GraphicsPipelineConfig config{};
            config.render_pass = swap_chain->default_render_pass();

            // Shader paths - try relative to executable first, then current directory
            std::vector<std::string> shader_paths = {
                "shaders/triangle.vert.spv",
                "../shaders/triangle.vert.spv",
                "../../shaders/triangle.vert.spv"
            };

            std::string vert_path, frag_path;
            for (const auto& path : shader_paths)
            {
                if (std::filesystem::exists(path))
                {
                    vert_path = path;
                    frag_path = std::filesystem::path(path).parent_path().string() + "/triangle.frag.spv";
                    logger::info("Found shaders at: " + path);
                    break;
                }
            }

            if (vert_path.empty())
            {
                logger::error("Could not find shader files! Searched in:");
                for (const auto& path : shader_paths)
                {
                    logger::error("  - " + path);
                }
                throw std::runtime_error("Shader files not found");
            }

            config.vertex_shader_path   = vert_path;
            config.fragment_shader_path = frag_path;

            // Vertex input
            config.vertex_bindings = {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            };
            config.vertex_attributes = {
                {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
            };

            // Primitive topology
            config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // Rasterization
            config.polygon_mode = VK_POLYGON_MODE_FILL;
            config.cull_mode    = VK_CULL_MODE_NONE; // Disable culling for simple triangle
            config.front_face   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            // Depth (disabled for simple triangle)
            config.depth_test_enable  = false;
            config.depth_write_enable = false;

            // Blending
            config.blend_enable = false;

            pipeline_ = std::make_unique<vulkan::GraphicsPipeline>(device, config);
        }

        void recreate_resources(uint32_t width, uint32_t height)
        {
            (void)width;
            (void)height;

            // Wait for device idle
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Recreate framebuffers
            framebuffer_pool_->clear();
            create_framebuffers();

            // Recreate pipeline with new render pass
            pipeline_.reset();
            create_pipeline();
        }

    private:
        std::unique_ptr<vulkan::FrameSyncManager>  frame_sync_;
        std::unique_ptr<vulkan::FramebufferPool>   framebuffer_pool_;
        std::unique_ptr<vulkan::RenderCommandPool> cmd_pool_;
        std::vector<vulkan::RenderCommandBuffer>   cmd_buffers_;
        std::unique_ptr<vulkan::GraphicsPipeline>  pipeline_;
        std::unique_ptr<vulkan::Buffer>            vertex_buffer_;
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