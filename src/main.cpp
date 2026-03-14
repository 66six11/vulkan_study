#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "core/math/Camera.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/resources/DepthBuffer.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

// Render Graph includes
#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/render_graph/CubeRenderPass.hpp"

// Material system includes
#include "rendering/material/Material.hpp"
#include "rendering/material/MaterialLoader.hpp"

// Mesh loading includes
#include "rendering/resources/Mesh.hpp"
#include "rendering/resources/ObjLoader.hpp"

// Platform includes
#include "platform/input/InputManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>

using namespace vulkan_engine;

// Forward declaration for Render Graph test
namespace vulkan_engine::rendering
{
    void test_render_graph_resource_management(std::shared_ptr<vulkan::DeviceManager> device);
}

// 3D vertex structure with UV
struct Vertex
{
    float position[3];
    float color[3];
    float uv[2];
};

// Cube vertices with colors and UVs
const std::vector<Vertex> cube_vertices = {
    // Front face (red) - UVs mapped to show grid properly
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    // Back face (green)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    // Top face (blue)
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    // Bottom face (yellow)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    // Right face (magenta)
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    // Left face (cyan)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

// Cube indices
const std::vector<uint16_t> cube_indices = {
    0,
    1,
    2,
    0,
    2,
    3,
    // Front
    5,
    7,
    6,
    5,
    4,
    7,
    // Back
    8,
    10,
    9,
    8,
    11,
    10,
    // Top
    12,
    13,
    14,
    12,
    14,
    15,
    // Bottom
    16,
    17,
    18,
    16,
    18,
    19,
    // Right
    20,
    23,
    22,
    20,
    22,
    21 // Left
};

class CubeApplication : public application::ApplicationBase
{
    public:
        explicit CubeApplication(const application::ApplicationConfig& config)
            : application::ApplicationBase(config)
        {
        }

        bool on_initialize() override
        {
            logger::info("Initializing Cube Application with Render Graph");

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain)
            {
                logger::error("Device or swap chain not initialized");
                return false;
            }

            width_  = config().width;
            height_ = config().height;

            // Create frame sync manager
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2, swap_chain->image_count());

            // Create depth buffer
            depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device, width_, height_);

            // Create render pass with depth attachment
            swap_chain->create_render_pass_with_depth(depth_buffer_->format());

            // Create framebuffer pool
            create_framebuffers();

            // Create command pool and buffers
            cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device,
                                                                    0,
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            cmd_buffers_ = cmd_pool_->allocate(2);

            // Try to load OBJ model, fallback to cube if not found
            rendering::ObjLoader obj_loader;
            std::string          obj_path = "D:/TechArt/Vulkan/model/未命名.obj";

            if (obj_loader.can_load(obj_path))
            {
                logger::info("Loading OBJ model: " + obj_path);
                rendering::MeshData mesh_data = obj_loader.load(obj_path);

                if (!mesh_data.is_empty())
                {
                    mesh_ = std::make_unique<rendering::Mesh>();
                    mesh_->upload(device, mesh_data);
                    logger::info("OBJ model loaded and uploaded to GPU");
                }
                else
                {
                    logger::warn("Failed to load OBJ model, using default cube");
                    create_vertex_buffer();
                    create_index_buffer();
                }
            }
            else
            {
                logger::info("No OBJ model found at " + obj_path + ", using default cube");
                create_vertex_buffer();
                create_index_buffer();
            }

            // Initialize Material System
            logger::info("Initializing Material System...");
            material_loader_ = std::make_unique<rendering::MaterialLoader>(device);
            material_loader_->set_base_directory("D:/TechArt/Vulkan/materials/");
            material_loader_->set_texture_directory("D:/TechArt/Vulkan/");

            // Load materials (need render pass from swap chain)
            materials_.push_back(material_loader_->load("metal.json", swap_chain->default_render_pass()));
            materials_.push_back(material_loader_->load("plastic.json", swap_chain->default_render_pass()));
            materials_.push_back(material_loader_->load("emissive.json", swap_chain->default_render_pass()));
            materials_.push_back(material_loader_->load("textured.json", swap_chain->default_render_pass()));
            materials_.push_back(material_loader_->load("normal_vis.json", swap_chain->default_render_pass()));

            // Set initial material
            if (!materials_.empty())
            {
                current_material_ = materials_[0];
                logger::info("Loaded " + std::to_string(materials_.size()) + " materials");
                logger::info("Press M to switch materials");
            }

            // Initialize Render Graph (uses loaded materials)
            initialize_render_graph();

            // Test Render Graph resource management
            logger::info("\nRunning Render Graph Resource Management Test...");
            rendering::test_render_graph_resource_management(device);

            // Register swap chain resize callback
            swap_chain->on_recreate([this](uint32_t width, uint32_t height)
            {
                recreate_resources(width, height);
            });

            // Initialize camera
            camera_.set_target(glm::vec3(0.0f, 0.0f, 0.0f));
            camera_.set_distance(3.0f);
            camera_.set_rotation(45.0f, -30.0f);
            camera_.set_distance_limits(1.0f, 10.0f);

            logger::info("Cube Application initialized successfully with Render Graph");
            logger::info("Camera controls: Left mouse drag to rotate, scroll to zoom");
            return true;
        }

        void on_shutdown() override
        {
            logger::info("Shutting down Cube Application");

            // Wait for device idle
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Cleanup
            cleanup_resources();
        }

        void on_update(float delta_time) override
        {
            (void)delta_time;

            // Handle camera input
            if (input_manager())
            {
                // Mouse drag for orbit rotation
                if (input_manager()->is_mouse_button_pressed(platform::MouseButton::Left))
                {
                    auto [delta_x, delta_y] = input_manager()->mouse_delta();

                    if (delta_x != 0.0 || delta_y != 0.0)
                    {
                        camera_.on_mouse_drag(static_cast<float>(delta_x), static_cast<float>(delta_y));
                    }
                }

                // Mouse scroll for zoom
                double scroll = input_manager()->scroll_delta();
                if (scroll != 0.0)
                {
                    camera_.on_mouse_scroll(static_cast<float>(scroll));
                }

                // Check for material switching (M key)
                if (input_manager()->is_key_just_pressed(platform::Key::M))
                {
                    if (!materials_.empty())
                    {
                        current_material_index_ = (current_material_index_ + 1) % materials_.size();
                        current_material_       = materials_[current_material_index_];
                        logger::info("Switched to material: " + current_material_->name());

                        // Update CubeRenderPass material
                        if (cube_pass_)
                        {
                            cube_pass_->set_material(current_material_);
                        }
                    }
                }
            }
        }

        void on_render() override
        {
            if (!frame_sync_ || !swap_chain()) return;

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            // Wait for current frame
            frame_sync_->wait_and_reset_current_fence();

            // Acquire next image
            uint32_t image_index = 0;
            bool     acquired    = swap_chain->acquire_next_image(
                                                           frame_sync_->get_current_image_available_semaphore().handle(),
                                                           VK_NULL_HANDLE,
                                                           image_index);

            if (!acquired)
            {
                return;
            }

            uint32_t frame_index = frame_sync_->get_current_frame();

            // Update uniform buffer
            update_mvp_matrix(frame_index);

            // Record and execute render graph
            record_and_execute_frame(frame_index, image_index);

            // Present
            VkSemaphore present_semaphore = frame_sync_->get_render_finished_semaphore(image_index).handle();
            swap_chain->present(device->graphics_queue(), image_index, present_semaphore);

            // Next frame
            frame_sync_->next_frame();
        }

    private:
        void initialize_render_graph()
        {
            logger::info("Initializing Render Graph for Cube Rendering");

            // Initialize render graph with device
            render_graph_.initialize(device_manager());

            // Import depth buffer as external resource
            auto depth_handle = render_graph_.import_image(
                                                           depth_buffer_->image(),
                                                           depth_buffer_->view(),
                                                           depth_buffer_->format(),
                                                           width_,
                                                           height_,
                                                           "depth_buffer"
                                                          );

            // Create cube render pass (will be reused each frame)
            rendering::CubeRenderPass::Config cube_config;
            cube_config.name = "CubeRenderPass";

            // Use mesh if loaded, otherwise use default cube buffers
            if (mesh_ && mesh_->is_uploaded())
            {
                cube_config.vertex_buffer = mesh_->vertex_buffer();
                cube_config.index_buffer  = mesh_->index_buffer();
                cube_config.index_count   = mesh_->index_count();
                cube_config.index_type    = VK_INDEX_TYPE_UINT32; // OBJ uses 32-bit indices
                logger::info("Using OBJ mesh for rendering (32-bit indices)");
            }
            else
            {
                cube_config.vertex_buffer = vertex_buffer_.get();
                cube_config.index_buffer  = index_buffer_.get();
                cube_config.index_count   = static_cast<uint32_t>(cube_indices.size());
                cube_config.index_type    = VK_INDEX_TYPE_UINT16; // Cube uses 16-bit indices
                logger::info("Using default cube for rendering (16-bit indices)");
            }
            cube_config.material_ref  = current_material_;
            cube_config.color_output  = rendering::ImageHandle(); // Will be set per-frame
            cube_config.depth_output  = depth_handle;
            cube_config.width         = width_;
            cube_config.height        = height_;

            logger::info("CubeRenderPass using material: " + current_material_->name());

            auto cube_pass = std::make_unique<rendering::CubeRenderPass>(cube_config);
            cube_pass_     = cube_pass.get(); // Save pointer for frame updates

            // Build render graph
            render_graph_.builder().add_node(std::move(cube_pass));

            // Compile render graph once
            render_graph_.compile();

            logger::info("Render Graph initialized with 1 pass (reused each frame)");
        }

        void record_and_execute_frame(uint32_t frame_index, uint32_t image_index)
        {
            auto& cmd        = cmd_buffers_[frame_index];
            auto  swap_chain = this->swap_chain();

            // Reset and begin command buffer
            cmd.reset();
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Create render context with current framebuffer
            rendering::RenderContext ctx;
            ctx.frame_index = frame_index;
            ctx.image_index = image_index;
            ctx.width       = width_;
            ctx.height      = height_;
            ctx.render_pass = swap_chain->default_render_pass();
            ctx.framebuffer = framebuffer_pool_->get_framebuffer(image_index)->handle();
            ctx.device      = device_manager();

            // Execute render graph (already compiled in initialize)
            render_graph_.execute(cmd, ctx);

            cmd.end();

            // Submit
            VkSemaphore          wait_semaphores[]   = {frame_sync_->get_current_image_available_semaphore().handle()};
            VkSemaphore          signal_semaphores[] = {frame_sync_->get_render_finished_semaphore(image_index).handle()};
            VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            VkCommandBuffer cmd_handle = cmd.handle();

            VkSubmitInfo submit_info{};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 1;
            submit_info.pWaitSemaphores      = wait_semaphores;
            submit_info.pWaitDstStageMask    = wait_stages;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &cmd_handle;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = signal_semaphores;

            VkFence fence = frame_sync_->get_current_fence().handle();
            vkQueueSubmit(device_manager()->graphics_queue(), 1, &submit_info, fence);
        }

        void create_framebuffers()
        {
            auto                     swap_chain = this->swap_chain();
            std::vector<VkImageView> image_views;
            image_views.reserve(swap_chain->image_count());
            for (uint32_t i = 0; i < swap_chain->image_count(); ++i)
            {
                image_views.push_back(swap_chain->get_image(i).view);
            }

            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device_manager());
            framebuffer_pool_->create_for_swap_chain(
                                                     swap_chain->default_render_pass(),
                                                     image_views,
                                                     width_,
                                                     height_,
                                                     depth_buffer_->view());
        }

        void create_vertex_buffer()
        {
            auto         device      = device_manager();
            VkDeviceSize buffer_size = sizeof(cube_vertices[0]) * cube_vertices.size();

            vertex_buffer_ = std::make_unique<vulkan::Buffer>(
                                                              device,
                                                              buffer_size,
                                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void* data = vertex_buffer_->map();
            memcpy(data, cube_vertices.data(), static_cast<size_t>(buffer_size));
            vertex_buffer_->unmap();
        }

        void create_index_buffer()
        {
            auto         device      = device_manager();
            VkDeviceSize buffer_size = sizeof(cube_indices[0]) * cube_indices.size();

            index_buffer_ = std::make_unique<vulkan::Buffer>(
                                                             device,
                                                             buffer_size,
                                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void* data = index_buffer_->map();
            memcpy(data, cube_indices.data(), static_cast<size_t>(buffer_size));
            index_buffer_->unmap();
        }

        void update_mvp_matrix(uint32_t /*frame_index*/)
        {
            // Model matrix (identity - cube stays at origin)
            glm::mat4 model = glm::mat4(1.0f);

            // View matrix from orbit camera
            glm::mat4 view = camera_.get_view_matrix();

            // Projection matrix
            glm::mat4 proj = camera_.get_projection_matrix(
                                                           45.0f,
                                                           static_cast<float>(width_) / static_cast<float>(height_),
                                                           0.1f,
                                                           100.0f);

            glm::mat4 mvp = proj * view * model;

            // Update CubeRenderPass MVP
            if (cube_pass_)
            {
                cube_pass_->set_mvp_matrix(mvp);
            }
        }

        void recreate_resources(uint32_t width, uint32_t height)
        {
            logger::info("Recreating resources for " + std::to_string(width) + "x" + std::to_string(height));

            width_  = width;
            height_ = height;

            vkDeviceWaitIdle(device_manager()->device());

            depth_buffer_.reset();
            depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device_manager(), width_, height_);

            swap_chain()->create_render_pass_with_depth(depth_buffer_->format());

            framebuffer_pool_.reset();
            create_framebuffers();
        }

        void cleanup_resources()
        {
            // Resources are automatically cleaned up by their destructors
            // (RAII pattern)
        }

        // Member variables
        uint32_t width_  = 0;
        uint32_t height_ = 0;

        // Orbit camera for viewing the cube
        core::OrbitCamera camera_;

        std::unique_ptr<vulkan::FrameSyncManager>  frame_sync_;
        std::unique_ptr<vulkan::DepthBuffer>       depth_buffer_;
        std::unique_ptr<vulkan::FramebufferPool>   framebuffer_pool_;
        std::unique_ptr<vulkan::RenderCommandPool> cmd_pool_;
        std::vector<vulkan::RenderCommandBuffer>   cmd_buffers_;
        std::unique_ptr<vulkan::Buffer>            vertex_buffer_;
        std::unique_ptr<vulkan::Buffer>            index_buffer_;

        // Mesh (loaded from OBJ or default cube)
        std::unique_ptr<rendering::Mesh> mesh_;

        // Render Graph
        rendering::RenderGraph     render_graph_;
        rendering::CubeRenderPass* cube_pass_ = nullptr; // Owned by render_graph_

        // Material system
        std::unique_ptr<rendering::MaterialLoader>        material_loader_;
        std::shared_ptr<rendering::Material>              current_material_;
        std::vector<std::shared_ptr<rendering::Material>> materials_;
        size_t                                            current_material_index_ = 0;
};

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
        application::ApplicationConfig config{
            .title = "Vulkan Engine - Render Graph Demo",
            .width = 1280,
            .height = 720,
            .vsync = true,
            .enable_validation = true,
            .enable_profiling = true
        };

        auto app = std::make_unique<CubeApplication>(config);

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