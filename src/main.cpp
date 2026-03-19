#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "platform/filesystem/PathUtils.hpp"
#include "core/math/Camera.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/DepthBuffer.hpp"
#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"
#include "vulkan/utils/CoordinateTransform.hpp"
#include "vulkan/pipelines/RenderPassManager.hpp"

// Viewport includes
#include "rendering/Viewport.hpp"
#include "rendering/resources/RenderTarget.hpp"

// Editor includes
#include "editor/Editor.hpp"
#include "editor/ImGuiManager.hpp"

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

            // Create depth buffer FIRST (needed by Editor)
            depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device, width_, height_);

            // Initialize RenderPassManager and create render pass
            render_pass_manager_             = std::make_unique<vulkan::RenderPassManager>(device);
            VkRenderPass present_render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                 swap_chain->format(),
                 depth_buffer_->format()
                );
            if (present_render_pass == VK_NULL_HANDLE)
            {
                logger::error("Failed to create render pass with depth");
                return false;
            }
            logger::info("Render pass created successfully");

            // IMPORTANT: Update SwapChain's default render pass to match RenderPassManager
            // This ensures Editor and Material use the same render pass
            swap_chain->create_render_pass_with_depth(depth_buffer_->format());

            // Initialize Viewport and RenderTarget FIRST (needed by materials and Editor)
            initialize_viewport();

            // Initialize Editor (with viewport for ImGui display)
            editor_ = std::make_unique<editor::Editor>();
            editor_->initialize(window(), device, swap_chain, render_target_, viewport_);
            editor_->set_deferred_resize_enabled(true); // Enable deferred resize for resource safety

            // Set viewport resize callback to mark pending resize
            // Actual resize will be processed at end of frame in on_render()
            editor_->set_viewport_resize_callback([this](uint32_t width, uint32_t height)
            {
                // Mark resize as pending - actual resize will happen at frame boundary
                // to avoid resource competition with ongoing rendering
                viewport_resize_pending_ = true;
                viewport_resize_width_   = width;
                viewport_resize_height_  = height;
                logger::info("Viewport resize marked as pending: " + std::to_string(width) + "x" + std::to_string(height));
            });

            // Create frame sync manager (hybrid per-frame fence + per-image semaphore)
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2);

            // Resize per-image render_finished semaphores for swapchain
            if (swap_chain)
            {
                frame_sync_->resize_render_finished_semaphores(swap_chain->image_count());
            }

            // Create framebuffer pool for swap chain images
            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device);
            create_framebuffers();

            // Create command pool and buffers
            // Note: Scene command buffers are per-frame (not per-image)
            cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device,
                                                                    0,
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            cmd_buffers_ = cmd_pool_->allocate(2); // MAX_FRAMES_IN_FLIGHT

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
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Shutdown editor first
            if (editor_)
            {
                editor_->shutdown();
                editor_.reset();
            }

            // Clean up all Vulkan resources
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

            // Wait for previous frame to complete (CPU-GPU synchronization)
            // This ensures command buffers can be safely reset and reused
            frame_sync_->wait_and_reset_current_frame_fence();

            // Acquire next image from swap chain
            // Note: acquire semaphore is per-frame (not per-image) because we don't know
            // which image we'll get until after acquisition
            uint32_t image_index = 0;
            bool     acquired    = swap_chain->acquire_next_image(
                                                           frame_sync_->get_current_acquire_semaphore().handle(),
                                                           VK_NULL_HANDLE,
                                                           image_index);

            if (!acquired)
            {
                return;
            }

            uint32_t frame_index = frame_sync_->current_frame();

            // Begin editor frame (ImGui)
            editor_->begin_frame();

            // Render scene to viewport texture
            update_mvp_matrix();

            // Record scene command buffer
            VkCommandBuffer scene_cmd = record_scene_command_buffer(frame_index);

            if (scene_cmd != VK_NULL_HANDLE)
            {
                // Submit scene rendering and signal scene_finished semaphore
                VkSemaphore signal_semaphores[] = {frame_sync_->get_current_scene_finished_semaphore().handle()};

                VkSubmitInfo submit_info{};
                submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.commandBufferCount   = 1;
                submit_info.pCommandBuffers      = &scene_cmd;
                submit_info.signalSemaphoreCount = 1;
                submit_info.pSignalSemaphores    = signal_semaphores;

                // Scene submission - no fence needed, ImGui submit will use frame_fence
                vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
            }
            else
            {
                logger::warn("Scene command buffer is null, skipping scene render");
            }

            // End editor frame (renders ImGui)
            editor_->end_frame(image_index);

            // Record ImGui rendering to swap chain
            VkCommandBuffer gui_cmd = record_imgui_command_buffer(frame_index, image_index);

            // Submit ImGui rendering
            // Wait for: 1) swap chain image available (per-frame acquire semaphore), 2) scene rendering finished
            VkSemaphore wait_semaphores[] = {
                frame_sync_->get_current_acquire_semaphore().handle(),
                frame_sync_->get_current_scene_finished_semaphore().handle()
            };
            // Per-image: render finished semaphore for present
            VkSemaphore          signal_semaphores[] = {frame_sync_->get_render_finished_semaphore(image_index).handle()};
            VkPipelineStageFlags wait_stages[]       = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                // For acquire semaphore
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT // For scene_finished (viewport texture read)
            };

            VkSubmitInfo submit_info{};
            submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount   = 2;
            submit_info.pWaitSemaphores      = wait_semaphores;
            submit_info.pWaitDstStageMask    = wait_stages;
            submit_info.commandBufferCount   = 1;
            submit_info.pCommandBuffers      = &gui_cmd;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = signal_semaphores;

            // Use frame fence for command buffer lifecycle protection
            VkFence frame_fence = frame_sync_->get_current_frame_fence().handle();
            vkQueueSubmit(device->graphics_queue(), 1, &submit_info, frame_fence);

            // Present - wait for per-image render_finished semaphore
            VkSemaphore present_semaphore = frame_sync_->get_render_finished_semaphore(image_index).handle();
            swap_chain->present(device->graphics_queue(), image_index, present_semaphore);

            // Advance to next frame
            frame_sync_->advance_frame();

            // Process pending viewport resize at frame boundary
            // This ensures GPU has finished using the resources before we destroy them
            if (viewport_resize_pending_)
            {
                viewport_resize_pending_ = false;
                process_viewport_resize(viewport_resize_width_, viewport_resize_height_);
            }
        }

        void on_window_resize(const application::WindowResizeEvent& event) override
        {
            uint32_t width  = event.width;
            uint32_t height = event.height;

            if (width == 0 || height == 0)
                return;

            width_  = width;
            height_ = height;

            auto device     = device_manager();
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


            // 重建 render pass（RenderPassManager 会自动处理缓存）
            VkRenderPass present_render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                 swap_chain->format(),
                 depth_buffer_->format()
                );

            if (present_render_pass == VK_NULL_HANDLE)
            {
                logger::error("Failed to recreate render pass");
                return;
            }
            // 重建 Viewport 和 RenderTarget
            if (render_target_)
            {
                render_target_->resize(swap_chain->width(), swap_chain->height());
                // Recreate framebuffer for new size
                create_render_target_framebuffer();
            }

            if (viewport_)
            {
                viewport_->resize(swap_chain->width(), swap_chain->height());
            }
            // 重建 framebuffers

            framebuffer_pool_.reset();

            framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device);

            create_framebuffers();


            // 重建 FrameSyncManager（per-frame acquire + per-image render_finished）
            frame_sync_.reset();
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2);
            frame_sync_->resize_render_finished_semaphores(swap_chain->image_count());


            // 重建 ImGui（使用已创建的 present_render_pass）


            editor_->recreate_render_pass(present_render_pass, swap_chain->image_count());


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

            // Use new RenderTarget's render pass for material initialization
            logger::info("Material system using render_target_render_pass_: " +
                         std::to_string(reinterpret_cast<uint64_t>(render_target_render_pass_)));
            logger::info("RenderTarget depth_format: " + std::to_string(render_target_->depth_format()));

            if (render_target_render_pass_ != VK_NULL_HANDLE)
            {
                materials_.push_back(material_loader_->load("metal.json", render_target_render_pass_));
                materials_.push_back(material_loader_->load("plastic.json", render_target_render_pass_));
                materials_.push_back(material_loader_->load("emissive.json", render_target_render_pass_));
                materials_.push_back(material_loader_->load("textured.json", render_target_render_pass_));
                materials_.push_back(material_loader_->load("normal_vis.json", render_target_render_pass_));
            }
            else
            {
                logger::error("RenderTarget render pass not available for material initialization");
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

            // Use RenderPassManager to get present render pass with depth
            VkRenderPass present_render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                 swap_chain->format(),
                 depth_buffer_->format()
                );

            framebuffer_pool_->create_for_swap_chain(
                                                     present_render_pass,
                                                     image_views,
                                                     width_,
                                                     height_,
                                                     depth_buffer_->view());
        }

        VkCommandBuffer record_scene_command_buffer(uint32_t frame_index)
        {
            auto&           cmd        = cmd_buffers_[frame_index];
            VkCommandBuffer cmd_handle = cmd.handle();

            // Use new Viewport and RenderTarget
            if (!viewport_ || !render_target_ || render_target_->width() < 10 || render_target_->height() < 10)
            {
                logger::warn("RenderTarget too small or not available, skipping scene render");
                return VK_NULL_HANDLE;
            }

            vkResetCommandBuffer(cmd_handle, 0);
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Execute render graph using RenderTarget
            rendering::RenderContext ctx;
            ctx.frame_index = frame_index;
            ctx.image_index = 0;
            ctx.width       = render_target_->width();
            ctx.height      = render_target_->height();
            ctx.render_pass = render_target_render_pass_;
            ctx.framebuffer = render_target_->framebuffer_handle();
            ctx.device      = device_manager();

            render_graph_.execute(cmd, ctx);

            cmd.end();
            return cmd.handle();
        }

        VkCommandBuffer record_imgui_command_buffer(uint32_t frame_index, uint32_t image_index)
        {
            auto swap_chain = this->swap_chain();

            // Allocate imgui command buffers if not done (per-frame, not per-image)
            if (cmd_buffers_imgui_.empty())
            {
                cmd_buffers_imgui_ = cmd_pool_->allocate(2); // MAX_FRAMES_IN_FLIGHT
            }

            // Use frame_index for ImGui CB (per-frame architecture)
            if (frame_index >= cmd_buffers_imgui_.size())
            {
                logger::error("Invalid frame_index: " + std::to_string(frame_index) +
                              ", vector size: " + std::to_string(cmd_buffers_imgui_.size()));
                return VK_NULL_HANDLE;
            }

            // Record ImGui command buffer
            // Note: frame fence already ensures previous frame's work is complete
            auto& cmd = cmd_buffers_imgui_[frame_index];
            vkResetCommandBuffer(cmd.handle(), 0);
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Begin swap chain render pass
            // Use RenderPassManager to get present render pass with depth
            VkRenderPass present_render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                 swap_chain->format(),
                 depth_buffer_->format()
                );

            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass        = present_render_pass;
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

            // 使用新的 Viewport 获取宽高比
            float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);

            if (viewport_)
            {
                aspect_ratio = viewport_->aspect_ratio();
            }

            // 获取 OpenGL 风格的投影矩阵（Camera 现在是 API 无关的）
            glm::mat4 proj = camera_->get_projection_matrix(45.0f, aspect_ratio, 0.1f, 100.0f);

            // 转换为 Vulkan 投影矩阵（翻转 Y 轴）
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
            auto duration     = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time_).count();

            if (duration >= 1000)
            {
                current_fps_ = static_cast<float>(frame_count_) * 1000.0f / static_cast<float>(duration);
                frame_count_ = 0;
                last_time_   = current_time;
            }
        }

        void initialize_viewport()
        {
            auto device = device_manager();

            // Create VMA Allocator for RenderTarget memory management
            if (!vma_allocator_)
            {
                vulkan::memory::VmaAllocator::CreateInfo allocator_info;
                allocator_info.enableBudget = true;
                vma_allocator_              = std::make_shared<vulkan::memory::VmaAllocator>(device, allocator_info);
                logger::info("VMA Allocator created for RenderTarget");
            }

            // Create RenderTarget
            rendering::RenderTarget::CreateInfo rt_info;
            rt_info.width        = width_;
            rt_info.height       = height_;
            rt_info.color_format = VK_FORMAT_B8G8R8A8_UNORM;
            rt_info.depth_format = VK_FORMAT_D32_SFLOAT;
            rt_info.create_color = true;
            rt_info.create_depth = true;

            render_target_ = std::make_shared<rendering::RenderTarget>();
            render_target_->initialize(vma_allocator_, rt_info);
            logger::info("RenderTarget initialized: " + std::to_string(width_) + "x" + std::to_string(height_));

            // Create Viewport
            viewport_ = std::make_shared<rendering::Viewport>();
            viewport_->initialize(device, render_target_);
            logger::info("Viewport initialized");

            // Create RenderPass for RenderTarget (off-screen rendering)
            logger::info("Creating offscreen render pass for RenderTarget:");
            logger::info("  color_format: " + std::to_string(render_target_->color_format()));
            logger::info("  depth_format: " + std::to_string(render_target_->depth_format()));

            render_target_render_pass_ = render_pass_manager_->get_offscreen_render_pass(
                                                                                         render_target_->color_format(),
                                                                                         render_target_->depth_format()
                                                                                        );

            logger::info("Created render_target_render_pass_: " + std::to_string(reinterpret_cast<uint64_t>(render_target_render_pass_)));

            // Create Framebuffer for RenderTarget
            create_render_target_framebuffer();
        }

        void create_render_target_framebuffer()
        {
            if (!render_target_ || render_target_render_pass_ == VK_NULL_HANDLE)
                return;

            // 使用 RenderTarget 的 Framebuffer 管理（RAII）
            render_target_->create_framebuffer(render_target_render_pass_);

            if (render_target_->has_framebuffer())
            {
                logger::info("RenderTarget framebuffer created: " + std::to_string(render_target_->width()) + "x" +
                             std::to_string(render_target_->height()));
            }
            else
            {
                logger::error("Failed to create render target framebuffer");
            }
        }

        void cleanup_resources()
        {
            // Note: Avoid logging at start to prevent Unicode issues
            // 获取当前设备管理器
            auto device_manager_ptr = device_manager();
            if (!device_manager_ptr)
            {
                logger::error("DeviceManager is null in cleanup_resources()! Cannot clean up Vulkan resources.");
                return;
            }

            // 获取 VkDevice 句柄
            VkDevice vk_device = device_manager_ptr->device();
            if (vk_device == VK_NULL_HANDLE)
            {
                logger::error("VkDevice is null in cleanup_resources()!");
                return;
            }

            // 等待设备空闲，确保所有正在执行的命令完成
            vkDeviceWaitIdle(vk_device);

            // 1. 首先清理渲染图系统
            render_graph_.reset();
            cube_pass_ = nullptr;

            // 2. 清理 RenderTarget 的 Framebuffer
            // RenderTarget 会在 cleanup() 或析构时自动销毁 Framebuffer
            // 这里不需要手动操作，但为了清晰，显式调用 destroy_framebuffer
            if (render_target_)
            {
                render_target_->destroy_framebuffer();
            }
            render_target_render_pass_ = VK_NULL_HANDLE;

            // 3. Destroy materials and material loader (hold pipelines / descriptor sets)
            current_material_.reset();
            materials_.clear();
            material_loader_.reset();

            // 4. Destroy geometry buffers
            vertex_buffer_.reset();
            index_buffer_.reset();
            mesh_.reset();

            // 5. Destroy command buffers and pool
            cmd_buffers_.clear();
            cmd_buffers_imgui_.clear();
            cmd_pool_.reset();

            // 6. Destroy framebuffer pool (swap chain framebuffers)
            // CRITICAL: Must destroy BEFORE device is destroyed
            logger::info("Destroying FramebufferPool...");
            framebuffer_pool_.reset();
            logger::info("FramebufferPool destroyed");

            // 7. Destroy frame sync objects
            frame_sync_.reset();

            // 8. Destroy viewport / render target (Image, ImageView, Memory)
            viewport_.reset();
            render_target_.reset();

            // 9. Destroy depth buffer
            depth_buffer_.reset();

            // 10. Destroy render pass manager (owns all cached VkRenderPass objects)
            render_pass_manager_.reset();

            // 11. Destroy camera controller and camera (no Vulkan resources, but clean up)
            camera_controller_.reset();
            camera_.reset();
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

        // RenderPass manager
        std::unique_ptr<vulkan::RenderPassManager> render_pass_manager_;

        // VMA Allocator for memory management
        std::shared_ptr<vulkan::memory::VmaAllocator> vma_allocator_;

        // Viewport and RenderTarget
        std::shared_ptr<rendering::RenderTarget> render_target_;
        std::shared_ptr<rendering::Viewport>     viewport_;

        // RenderTarget's RenderPass (Framebuffer 由 RenderTarget 自己管理)
        VkRenderPass render_target_render_pass_ = VK_NULL_HANDLE;

        // Pending viewport resize (processed at frame boundary)
        bool     viewport_resize_pending_ = false;
        uint32_t viewport_resize_width_   = 0;
        uint32_t viewport_resize_height_  = 0;

        void process_viewport_resize(uint32_t width, uint32_t height)
        {
            if (!render_target_ || !viewport_ || render_target_render_pass_ == VK_NULL_HANDLE)
                return;

            // Wait for GPU to finish all operations before recreating resources
            // This is safe because we're at frame boundary (after present)
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Apply viewport resize (rebuilds RenderTarget's Image/ImageView)
            viewport_->apply_pending_resize();

            // Recreate framebuffer with new ImageView
            create_render_target_framebuffer();

            logger::info("Viewport resize processed at frame boundary: " +
                         std::to_string(width) + "x" + std::to_string(height));
        }
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