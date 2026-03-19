#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "platform/filesystem/PathUtils.hpp"
#include "core/math/Camera.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/utils/CoordinateTransform.hpp"

// Rendering Layer includes
#include "rendering/Renderer.hpp"
#include "rendering/Viewport.hpp"
#include "rendering/resources/RenderTarget.hpp"
#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/render_graph/CubeRenderPass.hpp"
#include "rendering/material/Material.hpp"
#include "rendering/material/MaterialLoader.hpp"
#include "rendering/resources/Mesh.hpp"
#include "rendering/resources/ObjLoader.hpp"
#include "rendering/camera/CameraController.hpp"

// Editor includes
#include "editor/Editor.hpp"

// Platform includes
#include "platform/input/InputManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

using namespace vulkan_engine;

// 3D vertex structure with UV
struct Vertex
{
    float position[3];
    float color[3];
    float uv[2];
};

// Cube vertices with colors and UVs
const std::vector<Vertex> cube_vertices = {
    // Front face (red)
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

class EditorApplication : public application::ApplicationBase
{
    public:
        explicit EditorApplication(const application::ApplicationConfig& config)
            : application::ApplicationBase(config)
        {
        }

        bool on_initialize() override
        {
            logger::info("Initializing Editor Application");

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain)
            {
                logger::error("Device or swap chain not initialized");
                return false;
            }

            width_  = config().width;
            height_ = config().height;

            // Initialize Renderer (encapsulates all Vulkan rendering logic)
            rendering::Renderer::Config renderer_config;
            renderer_config.width                = width_;
            renderer_config.height               = height_;
            renderer_config.enable_gpu_timing    = true;
            renderer_config.max_frames_in_flight = 2;

            renderer_ = std::make_unique<rendering::Renderer>();
            if (!renderer_->initialize(device, swap_chain, renderer_config))
            {
                logger::error("Failed to initialize Renderer");
                return false;
            }

            // Initialize Editor (with viewport for ImGui display)
            editor_ = std::make_unique<editor::Editor>();
            editor_->initialize(
                                window(),
                                device,
                                swap_chain,
                                renderer_->render_target(),
                                renderer_->viewport());
            editor_->set_deferred_resize_enabled(true);

            // Set viewport resize callback
            editor_->set_viewport_resize_callback([this](uint32_t width, uint32_t height)
            {
                renderer_->resize(width, height);
            });

            // Load mesh
            load_mesh();

            // Initialize Material System
            initialize_materials();

            // Initialize Render Graph
            initialize_render_graph();

            // Initialize camera
            camera_ = std::make_shared<core::OrbitCamera>();
            camera_->set_target(glm::vec3(0.0f, 0.0f, 0.0f));
            camera_->set_distance(3.0f);
            camera_->set_rotation(45.0f, -30.0f);
            camera_->set_distance_limits(1.0f, 10.0f);

            // Initialize CameraController
            rendering::OrbitCameraController::Config controller_config;
            controller_config.use_imgui_input      = false;
            controller_config.rotation_sensitivity = 0.5f;
            controller_config.zoom_speed           = 0.1f;
            controller_config.require_mouse_drag   = true;
            controller_config.rotate_button        = platform::MouseButton::Left;

            camera_controller_ = std::make_unique<rendering::OrbitCameraController>(controller_config);
            camera_controller_->attach_camera(camera_);
            camera_controller_->attach_input_manager(input_manager());

            // Initialize FPS timer
            last_time_   = std::chrono::high_resolution_clock::now();
            frame_count_ = 0;

            logger::info("Editor Application initialized successfully");
            return true;
        }

        void on_shutdown() override
        {
            // Shutdown renderer first (waits for GPU and cleans up resources)
            if (renderer_)
            {
                renderer_->shutdown();
                renderer_.reset();
            }

            // Shutdown editor
            if (editor_)
            {
                editor_->shutdown();
                editor_.reset();
            }

            // Clean up remaining resources
            cleanup_resources();
        }

        void on_update(float delta_time) override
        {
            (void)delta_time;

            // Update FPS
            update_fps();

            // Update CameraController
            if (camera_controller_)
            {
                camera_controller_->set_enabled(editor_->is_viewport_content_hovered());
                camera_controller_->update(delta_time);
            }

            // Material switching (M key)
            if (input_manager() && input_manager()->is_key_just_pressed(platform::Key::M))
            {
                if (!materials_.empty())
                {
                    current_material_index_ = (current_material_index_ + 1) % materials_.size();
                    current_material_       = materials_[current_material_index_];
                    logger::info("Switched to material: " + current_material_->name());

                    if (cube_pass_)
                    {
                        cube_pass_->set_material(current_material_);
                    }
                }
            }

            // Update editor stats
            editor::ImGuiManager::StatsData stats;
            stats.fps                = current_fps_;
            stats.frame_time         = 1000.0f / current_fps_;
            stats.gpu_render_time_ms = renderer_->get_gpu_render_time_ms();
            stats.triangle_count     = (mesh_ && mesh_->is_uploaded()) ? mesh_->index_count() / 3 : 12;
            stats.draw_calls         = 1; // Cube pass
            stats.current_material   = current_material_ ? current_material_->name() : "None";
            editor_->update_stats(stats);
        }

        void on_render() override
        {
            if (!renderer_ || !editor_)
                return;

            // Begin frame
            if (!renderer_->begin_frame())
            {
                return;
            }

            // Update MVP matrix
            update_mvp_matrix();

            // Begin editor frame (ImGui)
            editor_->begin_frame();

            // End editor frame - creates ImGui draw data
            editor_->end_frame(renderer_->current_image());

            // Render scene using Renderer
            renderer_->render_scene([this](vulkan::RenderCommandBuffer& cmd, const rendering::Renderer::FrameContext& ctx)
            {
                // Execute render graph for scene
                rendering::RenderContext render_ctx;
                render_ctx.frame_index = ctx.frame_index;
                render_ctx.image_index = ctx.image_index;
                render_ctx.width       = ctx.width;
                render_ctx.height      = ctx.height;

                // Get render target's image views for dynamic rendering
                auto render_target          = renderer_->render_target();
                render_ctx.color_image_view = render_target->color_image_view();
                render_ctx.depth_image_view = render_target->depth_image_view();
                render_ctx.device           = renderer_->device();

                renderer_->render_graph().execute(cmd, render_ctx);
            });

            // Render UI to swap chain (signals render_finished semaphore for presentation)
            renderer_->render_ui(*editor_);

            // End frame and present
            renderer_->end_frame();
        }

        void on_window_resize(const application::WindowResizeEvent& event) override
        {
            uint32_t width  = event.width;
            uint32_t height = event.height;

            if (width == 0 || height == 0)
                return;

            width_  = width;
            height_ = height;

            // Resize renderer (handles swap chain, framebuffers, etc.)
            if (renderer_)
            {
                renderer_->resize(width, height);
            }

            // Recreate editor render pass
            if (editor_ && swap_chain())
            {
                editor_->recreate_render_pass(VK_NULL_HANDLE, swap_chain()->image_count());
            }

            logger::info("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
        }

    private:
        void load_mesh()
        {
            auto                 device = device_manager();
            rendering::ObjLoader obj_loader;
            std::string          obj_path = "D:/TechArt/Vulkan/model/mesh.obj";

            if (obj_loader.can_load(obj_path))
            {
                logger::info("Loading OBJ model: " + obj_path);
                rendering::MeshData mesh_data = obj_loader.load(obj_path);

                if (!mesh_data.is_empty())
                {
                    mesh_ = std::make_unique<rendering::Mesh>();
                    mesh_->upload(device, mesh_data);
                    logger::info("OBJ model loaded: " + std::to_string(mesh_data.vertices.size()) + " vertices");
                }
                else
                {
                    logger::warn("Failed to load OBJ, using default cube");
                    create_default_cube();
                }
            }
            else
            {
                logger::info("OBJ not found, using default cube");
                create_default_cube();
            }
        }

        void create_default_cube()
        {
            auto device = device_manager();

            VkDeviceSize vertex_size = sizeof(cube_vertices[0]) * cube_vertices.size();
            vertex_buffer_           = std::make_unique<vulkan::Buffer>(
                                                                        device,
                                                                        vertex_size,
                                                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            void* vdata = vertex_buffer_->map();
            memcpy(vdata, cube_vertices.data(), static_cast<size_t>(vertex_size));
            vertex_buffer_->unmap();

            VkDeviceSize index_size = sizeof(cube_indices[0]) * cube_indices.size();
            index_buffer_           = std::make_unique<vulkan::Buffer>(
                                                             device,
                                                             index_size,
                                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            void* idata = index_buffer_->map();
            memcpy(idata, cube_indices.data(), static_cast<size_t>(index_size));
            index_buffer_->unmap();
        }

        void initialize_materials()
        {
            auto device = device_manager();
            logger::info("Initializing Material System...");

            material_loader_ = std::make_unique<rendering::MaterialLoader>(device);
            material_loader_->set_base_directory(core::PathUtils::materials_dir().string() + "/");
            material_loader_->set_texture_directory(core::PathUtils::project_root().string() + "/");

            // Get render target for material initialization
            auto render_target = renderer_->render_target();

            if (render_target)
            {
                logger::info("Material system using RenderTarget formats:");
                logger::info("  color_format: " + std::to_string(render_target->color_format()));
                logger::info("  depth_format: " + std::to_string(render_target->depth_format()));
            }

            // Load materials - they will use dynamic rendering (no render pass needed)
            // Get render target formats for dynamic rendering
            VkFormat color_format = render_target->color_format();
            VkFormat depth_format = render_target->depth_format();

            materials_.push_back(material_loader_->load("metal.json", color_format, depth_format));
            materials_.push_back(material_loader_->load("plastic.json", color_format, depth_format));
            materials_.push_back(material_loader_->load("emissive.json", color_format, depth_format));
            materials_.push_back(material_loader_->load("textured.json", color_format, depth_format));
            materials_.push_back(material_loader_->load("normal_vis.json", color_format, depth_format));

            if (!materials_.empty())
            {
                current_material_ = materials_[0];
                logger::info("Loaded " + std::to_string(materials_.size()) + " materials");
            }
        }

        void initialize_render_graph()
        {
            logger::info("Initializing Render Graph");

            auto device = device_manager();

            // Create cube render pass
            rendering::CubeRenderPass::Config cube_config;
            cube_config.name = "CubeRenderPass";

            if (mesh_ && mesh_->is_uploaded())
            {
                cube_config.vertex_buffer = mesh_->vertex_buffer();
                cube_config.index_buffer  = mesh_->index_buffer();
                cube_config.index_count   = mesh_->index_count();
                cube_config.index_type    = VK_INDEX_TYPE_UINT32;
            }
            else
            {
                cube_config.vertex_buffer = vertex_buffer_.get();
                cube_config.index_buffer  = index_buffer_.get();
                cube_config.index_count   = static_cast<uint32_t>(cube_indices.size());
                cube_config.index_type    = VK_INDEX_TYPE_UINT16;
            }

            cube_config.material_ref = current_material_;
            cube_config.width        = width_;
            cube_config.height       = height_;

            auto cube_pass = std::make_unique<rendering::CubeRenderPass>(cube_config);
            cube_pass_     = cube_pass.get();

            renderer_->render_graph_builder().add_node(std::move(cube_pass));
            renderer_->compile_render_graph();

            logger::info("Render Graph initialized");
        }

        void update_mvp_matrix()
        {
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 view  = camera_->get_view_matrix();

            // Get aspect ratio from viewport
            float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
            if (renderer_ && renderer_->viewport())
            {
                aspect_ratio = renderer_->viewport()->aspect_ratio();
            }

            // Get projection matrix and convert to Vulkan
            glm::mat4 proj        = camera_->get_projection_matrix(45.0f, aspect_ratio, 0.1f, 100.0f);
            glm::mat4 vulkan_proj = vulkan::CoordinateTransform::opengl_to_vulkan_projection(proj);

        if (cube_pass_)
        {
            cube_pass_->set_mvp_matrix(vulkan_proj * view * model);
        }
    }

    void update_fps()
    {
        frame_count_++;
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time_).count();

        if (duration >= 1000)
        {
            current_fps_ = static_cast<float>(frame_count_) * 1000.0f / static_cast<float>(duration);
            frame_count_ = 0;
            last_time_ = current_time;
        }
    }

    void cleanup_resources()
    {
        // Clean up resources that are not managed by Renderer
        current_material_.reset();
        materials_.clear();
        material_loader_.reset();

        cube_pass_ = nullptr;

        vertex_buffer_.reset();
        index_buffer_.reset();
        mesh_.reset();

        camera_controller_.reset();
        camera_.reset();
    }

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Renderer (encapsulates all Vulkan rendering)
    std::unique_ptr<rendering::Renderer> renderer_;

    // Editor
    std::unique_ptr<editor::Editor> editor_;

    // Camera
    std::shared_ptr<core::OrbitCamera> camera_;
    std::unique_ptr<rendering::OrbitCameraController> camera_controller_;

    // Mesh
    std::unique_ptr<rendering::Mesh> mesh_;
    std::unique_ptr<vulkan::Buffer> vertex_buffer_;
    std::unique_ptr<vulkan::Buffer> index_buffer_;

    // Render Graph
    rendering::CubeRenderPass* cube_pass_ = nullptr;

    // Materials
    std::unique_ptr<rendering::MaterialLoader> material_loader_;
    std::shared_ptr<rendering::Material> current_material_;
    std::vector<std::shared_ptr<rendering::Material>> materials_;
    size_t current_material_index_ = 0;

    // FPS tracking
    std::chrono::high_resolution_clock::time_point last_time_;
    uint32_t frame_count_ = 0;
    float current_fps_ = 0.0f;
};

int main(int /*argc*/, char* /*argv*/[])
{
    logger::info("中文打印测试");
    try
    {
        application::ApplicationConfig config{
            .title = "Vulkan Engine - Editor",
            .width = 1600,
            .height = 900,
            .vsync = true,
            .enable_validation = true,
            .enable_profiling = true
        };

        auto app = std::make_unique<EditorApplication>(config);

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
