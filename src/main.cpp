#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "core/math/Camera.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/DepthBuffer.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

// Editor includes
#include "editor/Editor.hpp"
#include "editor/ImGuiManager.hpp"
#include "rendering/SceneViewport.hpp"

// Render Graph includes
#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/render_graph/CubeRenderPass.hpp"

// Material system includes
#include "rendering/material/Material.hpp"
#include "rendering/material/MaterialLoader.hpp"

// Mesh loading includes
#include "rendering/resources/Mesh.hpp"
#include "rendering/resources/ObjLoader.hpp"

// Platform includes
#include "platform/input/InputManager.hpp"

// Camera includes
#include "rendering/camera/CameraController.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

using namespace vulkan_engine;

// Forward declaration
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

            // Create depth buffer and render pass FIRST (needed by Editor)
            depth_buffer_            = std::make_unique<vulkan::DepthBuffer>(device, width_, height_);
            bool render_pass_created = swap_chain->create_render_pass_with_depth(depth_buffer_->format());
            if (!render_pass_created)
            {
                logger::error("Failed to create render pass with depth");
                return false;
            }
            logger::info("Render pass created successfully");

            // Initialize Editor (includes ImGui and SceneViewport) AFTER render pass is created
            editor_ = std::make_unique<editor::Editor>();
            editor_->initialize(window(), device, swap_chain);

            // Create frame sync manager
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2, swap_chain->image_count());

            // Create framebuffer pool for swap chain images
            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device);
            create_framebuffers();

            // Create command pool and buffers
            cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device,
                                                                    0,
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            cmd_buffers_ = cmd_pool_->allocate(swap_chain->image_count());

            // Load mesh
            load_mesh();

            // Initialize Material System
            initialize_materials();

            // Initialize Render Graph for viewport rendering
            initialize_render_graph();

            // Initialize camera
            camera_ = std::make_shared<core::OrbitCamera>();
            camera_->set_target(glm::vec3(0.0f, 0.0f, 0.0f));
            camera_->set_distance(3.0f);
            camera_->set_rotation(45.0f, -30.0f);
            camera_->set_distance_limits(1.0f, 10.0f);

            // Initialize CameraController
            rendering::OrbitCameraController::Config controller_config;
            controller_config.use_imgui_input      = false; // 使用 InputManager 而不是 ImGui 输入
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
            logger::info("Shutting down Editor Application");

            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            editor_->shutdown();
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
                // Enable/disable based on viewport content hover state
                camera_controller_->set_enabled(editor_->is_viewport_content_hovered());
                camera_controller_->update(delta_time);
            }

            // Material switching (M key) - works globally
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
            stats.fps              = current_fps_;
            stats.frame_time       = 1000.0f / current_fps_;
            stats.triangle_count   = (mesh_ && mesh_->is_uploaded()) ? mesh_->index_count() / 3 : 12;
            stats.draw_calls       = 1;
            stats.current_material = current_material_ ? current_material_->name() : "None";
            editor_->update_stats(stats);
        }

        void on_render() override
        {
            if (!frame_sync_ || !swap_chain() || !editor_)
                return;

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

            // Begin editor frame (ImGui)
            editor_->begin_frame();

            // Render scene to viewport texture
            update_mvp_matrix();
            VkCommandBuffer scene_cmd = record_scene_command_buffer(frame_index);
            if (scene_cmd != VK_NULL_HANDLE)
            {
                VkSubmitInfo submit_info{};
                submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.commandBufferCount = 1;
                submit_info.pCommandBuffers    = &scene_cmd;
                vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
                vkQueueWaitIdle(device->graphics_queue()); // Wait for scene to finish
            }

            // End editor frame (renders ImGui)
            editor_->end_frame(image_index);

            // Record ImGui rendering to swap chain
            VkCommandBuffer gui_cmd = record_imgui_command_buffer(image_index);

            // Submit ImGui rendering
            VkSemaphore          wait_semaphores[]   = {frame_sync_->get_current_image_available_semaphore().handle()};
            VkSemaphore          signal_semaphores[] = {frame_sync_->get_render_finished_semaphore(image_index).handle()};
            VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            VkSubmitInfo submit_info{};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 1;
            submit_info.pWaitSemaphores      = wait_semaphores;
            submit_info.pWaitDstStageMask    = wait_stages;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &gui_cmd;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = signal_semaphores;

            VkFence fence = frame_sync_->get_current_fence().handle();
            vkQueueSubmit(device->graphics_queue(), 1, &submit_info, fence);

            // Present
            VkSemaphore present_semaphore = frame_sync_->get_render_finished_semaphore(image_index).handle();
            swap_chain->present(device->graphics_queue(), image_index, present_semaphore);

            // Next frame
            frame_sync_->next_frame();
        }

        void on_window_resize(const application::WindowResizeEvent& event) override

        {
            uint32_t width = event.width;

            uint32_t height = event.height;


            if (width == 0 || height == 0)

                return;


            width_ = width;

            height_ = height;


            auto device = device_manager();

            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain)

                return;


            // 等待 GPU 完成所有操作

            vkDeviceWaitIdle(device->device());


            // 重建交换链（关键！必须与窗口尺寸匹配）

            if (!swap_chain->recreate())

            {
                logger::error("Failed to recreate swap chain");

                return;
            }


            // 重建 depth buffer（匹配新的交换链尺寸）

            depth_buffer_.reset();

            depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device, swap_chain->width(), swap_chain->height());


            // 重建 render pass

            swap_chain->create_render_pass_with_depth(depth_buffer_->format());


            // 重建 framebuffers

            framebuffer_pool_.reset();

            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device);

            create_framebuffers();


            // 重建 FrameSyncManager（image_count 可能已改变）

            frame_sync_.reset();

            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2, swap_chain->image_count());


            // 重建 ImGui

            editor_->recreate_render_pass(swap_chain->default_render_pass(), swap_chain->image_count());


            // 清除 ImGui command buffers（强制按新的 image count 重新分配）

            cmd_buffers_imgui_.clear();


            logger::info("Window resized to " + std::to_string(swap_chain->width()) + "x" +

                         std::to_string(swap_chain->height()) + ", images=" + std::to_string(swap_chain->image_count()));
        }

    private:
        void load_mesh()
        {
            auto                 device = device_manager();
            rendering::ObjLoader obj_loader;
            std::string          obj_path = "D:/TechArt/Vulkan/model/三角化网格.obj";

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
            material_loader_->set_base_directory("D:/TechArt/Vulkan/materials/");
            material_loader_->set_texture_directory("D:/TechArt/Vulkan/");

            // Create viewport render pass for material initialization
            auto viewport = editor_->viewport();
            if (viewport)
            {
                VkRenderPass vp_render_pass = viewport->render_pass();
                materials_.push_back(material_loader_->load("metal.json", vp_render_pass));
                materials_.push_back(material_loader_->load("plastic.json", vp_render_pass));
                materials_.push_back(material_loader_->load("emissive.json", vp_render_pass));
                materials_.push_back(material_loader_->load("textured.json", vp_render_pass));
                materials_.push_back(material_loader_->load("normal_vis.json", vp_render_pass));
            }

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
            render_graph_.initialize(device);

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
            cube_config.color_output = rendering::ImageHandle(); // Will be set
            cube_config.depth_output = rendering::ImageHandle(); // Will be set
            cube_config.width        = 1280;
            cube_config.height       = 720;

            auto cube_pass = std::make_unique<rendering::CubeRenderPass>(cube_config);
            cube_pass_     = cube_pass.get();

            render_graph_.builder().add_node(std::move(cube_pass));
            render_graph_.compile();

            logger::info("Render Graph initialized");
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

            framebuffer_pool_->create_for_swap_chain(
                                                     swap_chain->default_render_pass(),
                                                     image_views,
                                                     width_,
                                                     height_,
                                                     depth_buffer_->view());
        }

        VkCommandBuffer record_scene_command_buffer(uint32_t frame_index)
        {
            auto& cmd      = cmd_buffers_[frame_index];
            auto  viewport = editor_->viewport();

            if (!viewport || viewport->width() < 10 || viewport->height() < 10)
            {
                logger::warn("Viewport too small or not available, skipping scene render");
                return VK_NULL_HANDLE;
            }

            vkResetCommandBuffer(cmd.handle(), 0);
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Execute render graph (CubeRenderPass will handle begin/end render pass)
            rendering::RenderContext ctx;
            ctx.frame_index = frame_index;
            ctx.image_index = 0;
            ctx.width       = viewport->width();
            ctx.height      = viewport->height();
            ctx.render_pass = viewport->render_pass();
            ctx.framebuffer = viewport->framebuffer();
            ctx.device      = device_manager();

            render_graph_.execute(cmd, ctx);

            cmd.end();

            return cmd.handle();
        }

        VkCommandBuffer record_imgui_command_buffer(uint32_t image_index)
        {
            auto swap_chain = this->swap_chain();

            // Allocate imgui command buffers if not done
            if (cmd_buffers_imgui_.empty())
            {
                cmd_buffers_imgui_ = cmd_pool_->allocate(swap_chain->image_count());
            }

            // Check image_index is valid
            if (image_index >= cmd_buffers_imgui_.size())
            {
                logger::error("Invalid image_index: " + std::to_string(image_index) +
                              ", vector size: " + std::to_string(cmd_buffers_imgui_.size()));
                return VK_NULL_HANDLE;
            }

            auto& cmd = cmd_buffers_imgui_[image_index];

            vkResetCommandBuffer(cmd.handle(), 0);
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Begin swap chain render pass
            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass        = swap_chain->default_render_pass();
            render_pass_info.framebuffer       = framebuffer_pool_->get_framebuffer(image_index)->handle();
            render_pass_info.renderArea.offset = {0, 0};
            // Use swap chain extent to ensure correct size after resize
            auto extent                        = swap_chain->extent();
            render_pass_info.renderArea.extent = extent;

            VkClearValue clear_values[2];
            clear_values[0].color        = {0.2f, 0.2f, 0.2f, 1.0f};
            clear_values[1].depthStencil = {1.0f, 0};

            render_pass_info.clearValueCount = 2;
            render_pass_info.pClearValues    = clear_values;

            vkCmdBeginRenderPass(cmd.handle(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            // Render ImGui
            editor_->render_to_command_buffer(cmd.handle());

            vkCmdEndRenderPass(cmd.handle());
            cmd.end();

            return cmd.handle();
        }

        void update_mvp_matrix()
        {
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 view  = camera_->get_view_matrix();

            // 关键：投影矩阵的宽高比必须与最终显示区域的宽高比一致
            // 使用视窗的显示尺寸（而非实际渲染尺寸），处理延迟 resize 的情况
            float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);

            if (editor_ && editor_->viewport())
            {
                auto* viewport = editor_->viewport();
                // 使用 display_extent（目标显示尺寸），与 ImGui 视窗尺寸一致
                auto display_extent = viewport->display_extent();
                if (display_extent.height > 0)
                {
                    aspect_ratio = static_cast<float>(display_extent.width) /
                                   static_cast<float>(display_extent.height);
                }
            }

            glm::mat4 proj = camera_->get_projection_matrix(45.0f, aspect_ratio, 0.1f, 100.0f);

            if (cube_pass_)
            {
                cube_pass_->set_mvp_matrix(proj * view * model);
            }
        }

        void update_fps()
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

        void cleanup_resources()
        {
            // RAII handles cleanup
        }

        // Member variables
        uint32_t width_  = 0;
        uint32_t height_ = 0;

        // Editor
        std::unique_ptr<editor::Editor> editor_;
        VkRenderPass                    viewport_render_pass_ = VK_NULL_HANDLE;

        // Camera
        std::shared_ptr<core::OrbitCamera> camera_;

        // Frame sync
        std::unique_ptr<vulkan::FrameSyncManager> frame_sync_;

        // Depth buffer for swap chain
        std::unique_ptr<vulkan::DepthBuffer> depth_buffer_;

        // Framebuffer pool for swap chain images
        std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_;

        // Command buffers
        std::unique_ptr<vulkan::RenderCommandPool> cmd_pool_;
        std::vector<vulkan::RenderCommandBuffer>   cmd_buffers_;
        std::vector<vulkan::RenderCommandBuffer>   cmd_buffers_imgui_;

        // Mesh
        std::unique_ptr<rendering::Mesh> mesh_;
        std::unique_ptr<vulkan::Buffer>  vertex_buffer_;
        std::unique_ptr<vulkan::Buffer>  index_buffer_;

        // Render Graph
        rendering::RenderGraph     render_graph_;
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

        // Camera controller
        std::unique_ptr<rendering::OrbitCameraController> camera_controller_;
};

int main(int /*argc*/, char* /*argv*/[])
{
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