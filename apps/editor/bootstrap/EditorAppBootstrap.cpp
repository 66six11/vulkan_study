#include "EditorAppBootstrap.hpp"

// 浣跨敤寮曟搸鍛藉悕绌洪棿
using namespace vulkan_engine;

#include "engine/application/app/Application.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/platform/filesystem/PathUtils.hpp"
#include "engine/core/math/Camera.hpp"
#include "engine/rhi/vulkan/device/Device.hpp"
#include "engine/rhi/vulkan/device/SwapChain.hpp"
#include "engine/rhi/vulkan/resources/Buffer.hpp"
#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include "engine/rhi/vulkan/utils/CoordinateTransform.hpp"

#include "engine/rendering/ComposedRenderer.hpp"
#include "engine/rendering/Viewport.hpp"
#include "engine/rendering/resources/RenderTarget.hpp"
#include "engine/rendering/render_graph/RenderGraph.hpp"
#include "engine/rendering/render_graph/CubeRenderPass.hpp"
#include "engine/rendering/material/Material.hpp"
#include "engine/rendering/material/MaterialLoader.hpp"
#include "engine/rendering/resources/Mesh.hpp"
#include "engine/rendering/resources/ObjLoader.hpp"
#include "engine/rendering/camera/CameraController.hpp"

#include "engine/editor/Editor.hpp"
#include "engine/platform/input/InputManager.hpp"

#include "../demo/CubeData.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <chrono>

namespace editor::bootstrap
{
    // EditorAppConfig 瀹炵幇
    EditorAppConfig EditorAppConfig::parse(int argc, char* argv[])
    {
        EditorAppConfig config;

        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--width" && i + 1 < argc)
            {
                config.width = std::stoi(argv[++i]);
            }
            else if (arg == "--height" && i + 1 < argc)
            {
                config.height = std::stoi(argv[++i]);
            }
            else if (arg == "--no-vsync")
            {
                config.vsync = false;
            }
            else if (arg == "--no-validation")
            {
                config.enable_validation = false;
            }
            else if (arg == "--help")
            {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                        << "Options:\n"
                        << "  --width <n>       Set window width (default: 1600)\n"
                        << "  --height <n>      Set window height (default: 900)\n"
                        << "  --no-vsync        Disable VSync\n"
                        << "  --no-validation   Disable validation layers\n"
                        << "  --help            Show this help\n";
            }
        }

        return config;
    }

    // Pimpl 瀹炵幇绫?
    class EditorApplication::Impl
    {
        public:
            uint32_t width_  = 0;
            uint32_t height_ = 0;

            // ComposedRenderer (separate scene and UI pipelines)
            std::unique_ptr<rendering::ComposedRenderer> renderer_;

            // Editor
            std::unique_ptr<::vulkan_engine::editor::Editor> editor_;

            // Camera
            std::shared_ptr<core::OrbitCamera>                camera_;
            std::unique_ptr<rendering::OrbitCameraController> camera_controller_;

            // Mesh
            std::unique_ptr<rendering::Mesh> mesh_;
            std::unique_ptr<vulkan::Buffer>  vertex_buffer_;
            std::unique_ptr<vulkan::Buffer>  index_buffer_;

            // Render Graph
            rendering::CubeRenderPass* cube_pass_ = nullptr;

            // Materials
            std::unique_ptr<rendering::MaterialLoader>        material_loader_;
            std::shared_ptr<rendering::Material>              current_material_;
            std::vector<std::shared_ptr<rendering::Material>> materials_;
            size_t                                            current_material_index_ = 0;

            // FPS tracking
            std::chrono::high_resolution_clock::time_point last_time_;
            uint32_t                                       frame_count_ = 0;
            float                                          current_fps_ = 0.0f;

            void load_mesh(std::shared_ptr<vulkan::DeviceManager> device);
            void create_default_cube(std::shared_ptr<vulkan::DeviceManager> device);
            void initialize_materials(std::shared_ptr<vulkan::DeviceManager> device, rendering::ComposedRenderer* renderer);
            void initialize_render_graph(std::shared_ptr<vulkan::DeviceManager> device);
            void update_mvp_matrix();
            void update_fps();
            void cleanup_resources();
    };

    // EditorApplication 瀹炵幇
    EditorApplication::EditorApplication(const vulkan_engine::application::ApplicationConfig& config)
        : vulkan_engine::application::ApplicationBase(config)
        , impl_(std::make_unique<Impl>())
    {
    }

    bool EditorApplication::on_initialize()
    {
        logger::info("Initializing Editor Application");

        auto device     = device_manager();
        auto swap_chain = this->swap_chain();

        if (!device || !swap_chain)
        {
            logger::error("Device or swap chain not initialized");
            return false;
        }

        impl_->width_  = config().width;
        impl_->height_ = config().height;

        // Initialize ComposedRenderer (separate scene and UI pipelines)
        rendering::ComposedRenderer::Config renderer_config;
        renderer_config.scene_width          = impl_->width_;
        renderer_config.scene_height         = impl_->height_;
        renderer_config.enable_gpu_timing    = true;
        renderer_config.enable_vsync         = config().vsync;
        renderer_config.max_frames_in_flight = 2;

        impl_->renderer_ = std::make_unique<rendering::ComposedRenderer>();
        if (!impl_->renderer_->initialize(window(), device, swap_chain, renderer_config))
        {
            logger::error("Failed to initialize ComposedRenderer");
            return false;
        }

        // Initialize Editor
        impl_->editor_ = std::make_unique<::vulkan_engine::editor::Editor>();
        impl_->editor_->initialize(
                                   window(),
                                   device,
                                   swap_chain,
                                   impl_->renderer_->scene_render_target(),
                                   impl_->renderer_->scene_viewport());
        impl_->editor_->set_deferred_resize_enabled(true);

        // Set viewport resize callback
        impl_->editor_->set_viewport_resize_callback([this](uint32_t width, uint32_t height)
        {
            impl_->renderer_->resize_scene(width, height);
        });

        // Load mesh
        impl_->load_mesh(device);

        // Initialize Material System
        impl_->initialize_materials(device, impl_->renderer_.get());

        // Initialize Render Graph
        impl_->initialize_render_graph(device);

        // Initialize camera
        impl_->camera_ = std::make_shared<core::OrbitCamera>();
        impl_->camera_->set_target(glm::vec3(0.0f, 0.0f, 0.0f));
        impl_->camera_->set_distance(3.0f);
        impl_->camera_->set_rotation(45.0f, -30.0f);
        impl_->camera_->set_distance_limits(1.0f, 10.0f);

        // Initialize CameraController
        rendering::OrbitCameraController::Config controller_config;
        controller_config.use_imgui_input      = false;
        controller_config.rotation_sensitivity = 0.5f;
        controller_config.zoom_speed           = 0.1f;
        controller_config.require_mouse_drag   = true;
        controller_config.rotate_button        = platform::MouseButton::Left;

        impl_->camera_controller_ = std::make_unique<rendering::OrbitCameraController>(controller_config);
        impl_->camera_controller_->attach_camera(impl_->camera_);
        impl_->camera_controller_->attach_input_manager(input_manager());

        // Initialize FPS timer
        impl_->last_time_   = std::chrono::high_resolution_clock::now();
        impl_->frame_count_ = 0;

        logger::info("Editor Application initialized successfully");
        return true;
    }

    void EditorApplication::on_shutdown()
    {
        if (impl_->renderer_)
        {
            impl_->renderer_->shutdown();
            impl_->renderer_.reset();
        }

        if (impl_->editor_)
        {
            impl_->editor_->shutdown();
            impl_->editor_.reset();
        }

        impl_->cleanup_resources();
    }

    void EditorApplication::on_update(float delta_time)
    {
        (void)delta_time;

        impl_->update_fps();

        if (impl_->camera_controller_)
        {
            impl_->camera_controller_->set_enabled(impl_->editor_->is_viewport_content_hovered());
            impl_->camera_controller_->update(delta_time);
        }

        // Material switching (M key)
        if (input_manager() && input_manager()->is_key_just_pressed(platform::Key::M))
        {
            if (!impl_->materials_.empty())
            {
                impl_->current_material_index_ = (impl_->current_material_index_ + 1) % impl_->materials_.size();
                impl_->current_material_       = impl_->materials_[impl_->current_material_index_];
                logger::info("Switched to material: " + impl_->current_material_->name());

                if (impl_->cube_pass_)
                {
                    impl_->cube_pass_->set_material(impl_->current_material_);
                }
            }
        }

        // Update editor stats
        ::vulkan_engine::editor::ImGuiManager::StatsData stats;
        stats.fps                = impl_->current_fps_;
        stats.frame_time         = 1000.0f / impl_->current_fps_;
        stats.gpu_render_time_ms = impl_->renderer_->get_scene_gpu_time_ms();
        stats.triangle_count     = (impl_->mesh_ && impl_->mesh_->is_uploaded()) ? impl_->mesh_->index_count() / 3 : 12;
        stats.draw_calls         = 1;
        stats.current_material   = impl_->current_material_ ? impl_->current_material_->name() : "None";
        impl_->editor_->update_stats(stats);
    }

    void EditorApplication::on_render()
    {
        if (!impl_->renderer_ || !impl_->editor_)
            return;

        if (!impl_->renderer_->begin_frame())
        {
            return;
        }

        impl_->update_mvp_matrix();

        // 1. 棣栧厛娓叉煋鍦烘櫙鍒?RenderTarget
        impl_->renderer_->render_scene([this](vulkan::RenderCommandBuffer& cmd, const rendering::SceneRenderer::FrameContext& ctx)
        {
            rendering::RenderContext render_ctx;
            render_ctx.frame_index = ctx.frame_index;
            render_ctx.image_index = 0; // Scene doesn't have swap chain images
            render_ctx.width       = ctx.width;
            render_ctx.height      = ctx.height;

            auto render_target          = impl_->renderer_->scene_render_target();
            render_ctx.color_image_view = render_target->color_image_view();
            render_ctx.depth_image_view = render_target->depth_image_view();
            render_ctx.device           = impl_->renderer_->scene_renderer().device();

            impl_->renderer_->scene_render_graph().execute(cmd, render_ctx);
        });

        // 2. 鐒跺悗鍒涘缓 ImGui UI锛堝彲浠ラ噰鏍峰凡娓叉煋鐨勫満鏅汗鐞嗭級
        impl_->editor_->begin_frame();
        impl_->editor_->end_frame(impl_->renderer_->current_image());

        // 3. 娓叉煋UI鍒?SwapChain
        impl_->renderer_->render_ui(*impl_->editor_);
        impl_->renderer_->end_frame();
    }

    void EditorApplication::on_window_resize(const vulkan_engine::application::WindowResizeEvent& event)
    {
        uint32_t width  = event.width;
        uint32_t height = event.height;

        if (width == 0 || height == 0)
            return;

        impl_->width_  = width;
        impl_->height_ = height;

        if (impl_->renderer_)
        {
            impl_->renderer_->resize(width, height);
        }

        if (impl_->editor_ && swap_chain())
        {
            impl_->editor_->recreate_render_pass(VK_NULL_HANDLE, swap_chain()->image_count());
        }

        logger::info("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
    }

    // Impl 鏂规硶瀹炵幇
    void EditorApplication::Impl::load_mesh(std::shared_ptr<vulkan::DeviceManager> device)
    {
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
                create_default_cube(device);
            }
        }
        else
        {
            logger::info("OBJ not found, using default cube");
            create_default_cube(device);
        }
    }

    void EditorApplication::Impl::create_default_cube(std::shared_ptr<vulkan::DeviceManager> device)
    {
        using namespace demo;

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

    void EditorApplication::Impl::initialize_materials(std::shared_ptr<vulkan::DeviceManager> device, rendering::ComposedRenderer* renderer)
    {
        logger::info("Initializing Material System...");

        material_loader_ = std::make_unique<rendering::MaterialLoader>(device);
        material_loader_->set_base_directory(core::PathUtils::materials_dir().string() + "/");
        material_loader_->set_texture_directory(core::PathUtils::project_root().string() + "/");

        auto render_target = renderer->scene_render_target();

        if (render_target)
        {
            logger::info("Material system using RenderTarget formats:");
            logger::info("  color_format: " + std::to_string(render_target->color_format()));
            logger::info("  depth_format: " + std::to_string(render_target->depth_format()));
        }

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

    void EditorApplication::Impl::initialize_render_graph(std::shared_ptr<vulkan::DeviceManager> device)
    {
        (void)device; // 褰撳墠瀹炵幇涓笉闇€瑕佺洿鎺ヤ娇鐢?device
        logger::info("Initializing Render Graph");

        rendering::CubeRenderPass::Config cube_config;
        cube_config.name = "CubeRenderPass";

        if (mesh_&& mesh_
        ->
        is_uploaded()
        )
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
            cube_config.index_count   = static_cast<uint32_t>(demo::cube_indices.size());
            cube_config.index_type    = VK_INDEX_TYPE_UINT16;
        }

        cube_config.material_ref = current_material_;
        cube_config.width        = width_;
        cube_config.height       = height_;

        auto cube_pass = std::make_unique<rendering::CubeRenderPass>(cube_config);
        cube_pass_     = cube_pass.get();

        renderer_->scene_render_graph_builder().add_node(std::move(cube_pass));
        renderer_->compile_scene_render_graph();

        logger::info("Render Graph initialized");
    }

    void EditorApplication::Impl::update_mvp_matrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view  = camera_->get_view_matrix();

        float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
        if (renderer_&& renderer_
        ->
        scene_viewport()
        )
        {
            aspect_ratio = renderer_->scene_viewport()->aspect_ratio();
        }

        glm::mat4 proj        = camera_->get_projection_matrix(45.0f, aspect_ratio, 0.1f, 100.0f);
        glm::mat4 vulkan_proj = vulkan::CoordinateTransform::opengl_to_vulkan_projection(proj);

        if (cube_pass_)
        {
            cube_pass_->set_mvp_matrix(vulkan_proj * view * model);
        }
    }

    void EditorApplication::Impl::update_fps()
    {
        frame_count_++;
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration     = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time_).count();

        if (duration >= 1000)
        {
            current_fps_ = static_cast<float>(frame_count_) * 1000.0f / static_cast<float>(duration);
            frame_count_ = 0;
            last_time_   = current_time;
        }
    }

    void EditorApplication::Impl::cleanup_resources()
    {
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

    // 宸ュ巶鍑芥暟瀹炵幇
    std::unique_ptr<EditorApplication> create_editor_app(const EditorAppConfig& config)
    {
        vulkan_engine::application::ApplicationConfig app_config{
            .title = config.title,
            .width = config.width,
            .height = config.height,
            .vsync = config.vsync,
            .enable_validation = config.enable_validation,
            .enable_profiling = config.enable_profiling
        };

        return std::make_unique < EditorApplication > (app_config);
    }
} // namespace editor::bootstrap