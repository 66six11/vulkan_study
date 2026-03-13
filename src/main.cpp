#include "application/app/Application.hpp"
#include "core/utils/Logger.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/device/SwapChain.hpp"
#include "vulkan/pipelines/Pipeline.hpp"
#include "vulkan/pipelines/ShaderModule.hpp"
#include "vulkan/resources/Buffer.hpp"
#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/resources/DepthBuffer.hpp"
#include "vulkan/resources/UniformBuffer.hpp"
#include "vulkan/command/CommandBuffer.hpp"
#include "vulkan/sync/Synchronization.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

using namespace vulkan_engine;

// Uniform buffer object for MVP matrix
struct UniformBufferObject
{
    glm::mat4 mvp;
};

// 3D vertex structure
struct Vertex
{
    float position[3];
    float color[3];
};

// Cube vertices with colors
const std::vector<Vertex> cube_vertices = {
    // Front face (red)
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    // Back face (green)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    // Top face (blue)
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    // Bottom face (yellow)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
    // Right face (magenta)
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
    // Left face (cyan)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
};

// Cube indices - clockwise winding (from outside view) for VK_FRONT_FACE_CLOCKWISE
// Standard cube with vertices ordered for clockwise winding when viewed from outside
const std::vector<uint16_t> cube_indices = {
    // Front face (z = 0.5, red) - viewed from +Z
    // 3(-0.5,0.5) ---- 2(0.5,0.5)
    //    |                |
    // 0(-0.5,-0.5) ---- 1(0.5,-0.5)
    // CW: 0->1->2, 0->2->3
    0,
    1,
    2,
    0,
    2,
    3,

    // Back face (z = -0.5, green) - viewed from -Z
    // 7(-0.5,0.5) ---- 6(0.5,0.5)
    //    |                |
    // 4(-0.5,-0.5) ---- 5(0.5,-0.5)
    // CCW: 5->7->6, 5->4->7
    5,
    7,
    6,
    5,
    4,
    7,

    // Top face (y = 0.5, blue) - viewed from +Y (looking down)
    // 11(-0.5,0.5) ---- 10(0.5,0.5)
    //     |                |
    // 8(-0.5,-0.5) ---- 9(0.5,-0.5)
    // CW: 8->10->9, 8->11->10
    8,
    10,
    9,
    8,
    11,
    10,

    // Bottom face (y = -0.5, yellow) - viewed from -Y (looking up)
    // 15(-0.5,0.5) ---- 14(0.5,0.5)
    //     |                |
    // 12(-0.5,-0.5) ---- 13(0.5,-0.5)
    // CW: 12->13->14, 12->14->15
    12,
    13,
    14,
    12,
    14,
    15,

    // Right face (x = 0.5, magenta) - viewed from +X
    // 18(0.5,0.5) ---- 17(0.5,0.5) [z varies]
    //     |                |
    // 19(0.5,-0.5) ---- 16(0.5,-0.5)
    // CW: 16->17->18, 16->18->19
    16,
    17,
    18,
    16,
    18,
    19,

    // Left face (x = -0.5, cyan) - viewed from -X
    // 22(0.5,0.5) ---- 21(0.5,0.5) [z varies]
    //     |                |
    // 23(0.5,-0.5) ---- 20(0.5,-0.5)
    // CW: 20->23->22, 20->22->21
    20,
    23,
    22,
    20,
    22,
    21
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
            logger::info("Initializing Cube Application");

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            if (!device || !swap_chain)
            {
                logger::error("Device or swap chain not initialized");
                return false;
            }

            width_  = config().width;
            height_ = config().height;

            // Create frame sync manager (2 frames in flight, but semaphore per image)
            frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2, swap_chain->image_count());

            // Create depth buffer
            depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device, width_, height_);

            // Create render pass with depth attachment
            swap_chain->create_render_pass_with_depth(depth_buffer_->format());

            // Create framebuffer pool with depth attachment
            create_framebuffers();

            // Create command pool and buffers
            cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device,
                                                                    0,
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            cmd_buffers_ = cmd_pool_->allocate(2); // 2 frames in flight

            // Create vertex and index buffers
            create_vertex_buffer();
            create_index_buffer();

            // Create uniform buffer
            uniform_buffer_ = std::make_unique<vulkan::UniformBuffer<UniformBufferObject>>(device, 2);

            // Create descriptor set layout and pipeline layout
            create_descriptor_set_layout();
            create_pipeline_layout();

            // Create graphics pipeline
            create_pipeline();

            // Create descriptor sets
            create_descriptor_sets();

            // Register swap chain resize callback
            swap_chain->on_recreate([this](uint32_t width, uint32_t height)
            {
                recreate_resources(width, height);
            });

            // Initialize start time for animation
            start_time_ = std::chrono::high_resolution_clock::now();

            logger::info("Cube Application initialized successfully");
            return true;
        }

        void on_shutdown() override
        {
            logger::info("Shutting down Cube Application");

            // Wait for device idle before cleanup
            if (auto device = device_manager())
            {
                vkDeviceWaitIdle(device->device());
            }

            // Cleanup in reverse order
            vkDestroyDescriptorPool(device_manager()->device(), descriptor_pool_, nullptr);
            vkDestroyPipelineLayout(device_manager()->device(), pipeline_layout_, nullptr);
            vkDestroyDescriptorSetLayout(device_manager()->device(), descriptor_set_layout_, nullptr);
            uniform_buffer_.reset();
            index_buffer_.reset();
            vertex_buffer_.reset();
            cmd_buffers_.clear();
            cmd_pool_.reset();
            framebuffer_pool_.reset();
            depth_buffer_.reset();
            frame_sync_.reset();
        }

        void on_update(float delta_time) override
        {
            (void)delta_time;

            // Calculate elapsed time for rotation
            auto  now     = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - start_time_).count();

            // Update rotation angle
            rotation_angle_ = elapsed * 45.0f; // 45 degrees per second
        }

        void on_render() override
        {
            if (!frame_sync_ || !swap_chain()) return;

            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            // Wait for current frame's fence
            frame_sync_->wait_and_reset_current_fence();

            // Acquire next image
            uint32_t image_index = 0;
            bool     acquired    = swap_chain->acquire_next_image(
                                                                  frame_sync_->get_current_image_available_semaphore().handle(),
                                                                  VK_NULL_HANDLE,
                                                                  image_index);

            if (!acquired)
            {
                // Swap chain out of date, will be recreated
                return;
            }

            // Get current frame index (0 or 1 for double buffering)
            uint32_t frame_index = frame_sync_->get_current_frame();

            // Update uniform buffer with MVP matrix
            update_mvp_matrix(frame_index);

            // Record command buffer
            auto& cmd = cmd_buffers_[frame_index];
            cmd.reset();
            cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Begin render pass with depth attachment
            VkClearValue clear_values[2];
            clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Color clear
            clear_values[1].depthStencil = {1.0f, 0};                  // Depth clear

            VkRenderPassBeginInfo render_pass_info{};
            render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass        = swap_chain->default_render_pass();
            render_pass_info.framebuffer       = framebuffer_pool_->get_framebuffer(image_index)->handle();
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = {width_, height_};
            render_pass_info.clearValueCount   = 2;
            render_pass_info.pClearValues      = clear_values;

            vkCmdBeginRenderPass(cmd.handle(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            // Bind pipeline
            vkCmdBindPipeline(cmd.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->handle());

            // Set viewport and scissor
            VkViewport viewport{};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(width_);
            viewport.height   = static_cast<float>(height_);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd.handle(), 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {width_, height_};
            vkCmdSetScissor(cmd.handle(), 0, 1, &scissor);

            // Bind descriptor set
            vkCmdBindDescriptorSets(cmd.handle(),
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipeline_layout_,
                                    0,
                                    1,
                                    &descriptor_sets_[frame_index],
                                    0,
                                    nullptr);

            // Bind vertex buffer
            VkBuffer     vertex_buffers[] = {vertex_buffer_->handle()};
            VkDeviceSize offsets[]        = {0};
            vkCmdBindVertexBuffers(cmd.handle(), 0, 1, vertex_buffers, offsets);

            // Bind index buffer
            vkCmdBindIndexBuffer(cmd.handle(), index_buffer_->handle(), 0, VK_INDEX_TYPE_UINT16);

            // Draw indexed
            vkCmdDrawIndexed(cmd.handle(), static_cast<uint32_t>(cube_indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(cmd.handle());
            cmd.end();

            // Submit - use per-image render_finished semaphore to avoid reuse conflicts
            VkSemaphore          wait_semaphores[]   = {frame_sync_->get_current_image_available_semaphore().handle()};
            VkSemaphore          signal_semaphores[] = {frame_sync_->get_render_finished_semaphore(image_index).handle()};
            VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

            VkSubmitInfo    submit_info{};
            VkCommandBuffer cmd_handle = cmd.handle();

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

            // Present - use per-image semaphore
            VkSemaphore present_semaphore = frame_sync_->get_render_finished_semaphore(image_index).handle();
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

        void create_descriptor_set_layout()
        {
            VkDescriptorSetLayoutBinding ubo_layout_binding{};
            ubo_layout_binding.binding            = 0;
            ubo_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo_layout_binding.descriptorCount    = 1;
            ubo_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
            ubo_layout_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = 1;
            layout_info.pBindings    = &ubo_layout_binding;

            VkResult result = vkCreateDescriptorSetLayout(
                                                          device_manager()->device(),
                                                          &layout_info,
                                                          nullptr,
                                                          &descriptor_set_layout_);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create descriptor set layout");
            }
        }

        void create_pipeline_layout()
        {
            VkPipelineLayoutCreateInfo pipeline_layout_info{};
            pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount         = 1;
            pipeline_layout_info.pSetLayouts            = &descriptor_set_layout_;
            pipeline_layout_info.pushConstantRangeCount = 0;
            pipeline_layout_info.pPushConstantRanges    = nullptr;

            VkResult result = vkCreatePipelineLayout(
                                                     device_manager()->device(),
                                                     &pipeline_layout_info,
                                                     nullptr,
                                                     &pipeline_layout_);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create pipeline layout");
            }
        }

        void create_pipeline()
        {
            auto device     = device_manager();
            auto swap_chain = this->swap_chain();

            vulkan::GraphicsPipelineConfig config{};
            config.render_pass = swap_chain->default_render_pass();

            // Shader paths
            config.vertex_shader_path   = "shaders/triangle.vert.spv";
            config.fragment_shader_path = "shaders/triangle.frag.spv";

            // Pipeline layout
            config.layout = pipeline_layout_;

            // Vertex input - now 3D
            config.vertex_bindings = {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            };
            config.vertex_attributes = {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
            };

            // Primitive topology
            config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // Rasterization
            config.polygon_mode = VK_POLYGON_MODE_FILL;
            config.cull_mode    = VK_CULL_MODE_BACK_BIT;
            config.front_face   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            // Depth (enabled for 3D cube)
            config.depth_test_enable  = true;
            config.depth_write_enable = true;
            config.depth_compare_op   = VK_COMPARE_OP_LESS;

            // Blending
            config.blend_enable = false;

            pipeline_ = std::make_unique<vulkan::GraphicsPipeline>(device, config);
        }

        void create_descriptor_sets()
        {
            auto device = device_manager();

            // Create descriptor pool
            VkDescriptorPoolSize pool_size{};
            pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_size.descriptorCount = 2; // 2 frames

            VkDescriptorPoolCreateInfo pool_info{};
            pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes    = &pool_size;
            pool_info.maxSets       = 2;

            VkResult result = vkCreateDescriptorPool(device->device(), &pool_info, nullptr, &descriptor_pool_);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create descriptor pool");
            }

            // Allocate descriptor sets
            std::vector<VkDescriptorSetLayout> layouts(2, descriptor_set_layout_);
            VkDescriptorSetAllocateInfo        alloc_info{};
            alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool     = descriptor_pool_;
            alloc_info.descriptorSetCount = 2;
            alloc_info.pSetLayouts        = layouts.data();

            descriptor_sets_.resize(2);
            result = vkAllocateDescriptorSets(device->device(), &alloc_info, descriptor_sets_.data());
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate descriptor sets");
            }

            // Update descriptor sets
            for (size_t i = 0; i < 2; i++)
            {
                VkDescriptorBufferInfo buffer_info{};
                buffer_info.buffer = uniform_buffer_->buffer(static_cast<uint32_t>(i));
                buffer_info.offset = 0;
                buffer_info.range  = sizeof(UniformBufferObject);

                VkWriteDescriptorSet descriptor_write{};
                descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_write.dstSet          = descriptor_sets_[i];
                descriptor_write.dstBinding      = 0;
            descriptor_write.dstArrayElement     = 0;
            descriptor_write.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.descriptorCount = 1;
                descriptor_write.pBufferInfo     = &buffer_info;

            vkUpdateDescriptorSets(device->device(), 1, &descriptor_write, 0, nullptr);
        }
    }

    void update_mvp_matrix(uint32_t frame_index)
    {
        // Model matrix: rotate around Y and X axis
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(rotation_angle_), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation_angle_ * 0.5f), glm::vec3(1.0f, 0.0f, 0.0f));

        // View matrix: camera position
        glm::mat4 view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),  // Eye position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );

        // Projection matrix: perspective
        glm::mat4 proj = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width_) / static_cast<float>(height_),
            0.1f,
            100.0f
        );
        // GLM clip space fix for Vulkan
        proj[1][1] *= -1;

        UniformBufferObject ubo{};
        ubo.mvp = proj * view * model;

        uniform_buffer_->update(frame_index, ubo);
    }

    void recreate_resources(uint32_t width, uint32_t height)
    {
        logger::info("Recreating resources for " + std::to_string(width) + "x" + std::to_string(height));

        width_ = width;
        height_ = height;

        // Wait for device idle
        vkDeviceWaitIdle(device_manager()->device());

        // Recreate depth buffer
        depth_buffer_.reset();
        depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(device_manager(), width_, height_);

        // Recreate render pass with depth
        swap_chain()->create_render_pass_with_depth(depth_buffer_->format());

        // Recreate framebuffers
        framebuffer_pool_.reset();
        create_framebuffers();
    }

    // Member variables
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    float rotation_angle_ = 0.0f;
    std::chrono::high_resolution_clock::time_point start_time_;

    std::unique_ptr<vulkan::FrameSyncManager> frame_sync_;
    std::unique_ptr<vulkan::DepthBuffer> depth_buffer_;
    std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_;
    std::unique_ptr<vulkan::RenderCommandPool> cmd_pool_;
    std::vector<vulkan::RenderCommandBuffer> cmd_buffers_;
    std::unique_ptr<vulkan::Buffer> vertex_buffer_;
    std::unique_ptr<vulkan::Buffer> index_buffer_;
    std::unique_ptr<vulkan::UniformBuffer<UniformBufferObject>> uniform_buffer_;
    std::unique_ptr<vulkan::GraphicsPipeline> pipeline_;

    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets_;
};

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
        application::ApplicationConfig config{
            .title = "Vulkan Engine - Rotating Cube",
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