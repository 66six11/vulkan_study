#include "editor/ImGuiManager.hpp"
#include "rendering/SceneViewport.hpp"
#include "core/utils/Logger.hpp"
#include "vulkan/utils/VulkanError.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

namespace vulkan_engine::editor
{
    ImGuiManager::ImGuiManager() = default;

    ImGuiManager::~ImGuiManager()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    void ImGuiManager::initialize(
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<platform::Window>      window,
        VkRenderPass                           render_pass,
        uint32_t                               image_count)
    {
        device_ = device;
        window_ = window;
        (void)render_pass; // Not used in current ImGui version

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup ImGui style - Dark editor style
        ImGui::StyleColorsDark();
        ImGuiStyle& style    = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding  = 2.0f;

        // Setup platform bindings
        GLFWwindow* glfw_window = static_cast<GLFWwindow*>(window_->native_handle());
        ImGui_ImplGlfw_InitForVulkan(glfw_window, true);

        // Create descriptor pool
        create_descriptor_pool(image_count);

        // Setup Vulkan bindings
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = device->instance();
        init_info.PhysicalDevice            = device->physical_device();
        init_info.Device                    = device->device();
        init_info.QueueFamily               = device->graphics_queue_family();
        init_info.Queue                     = device->graphics_queue();
        init_info.PipelineCache             = VK_NULL_HANDLE;
        init_info.DescriptorPool            = descriptor_pool_;
        init_info.RenderPass                = render_pass; // <-- MISSING! Added now
        init_info.Subpass                   = 0;
        init_info.MinImageCount             = image_count;
        init_info.ImageCount                = image_count;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator                 = nullptr;
        init_info.CheckVkResultFn           = [](VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                logger::error("ImGui Vulkan error: " + std::to_string(result));
            }
        };

        ImGui_ImplVulkan_Init(&init_info);

        // Upload fonts using a temporary command buffer
        {
            VkCommandPool           command_pool = VK_NULL_HANDLE;
            VkCommandPoolCreateInfo pool_info    = {};
            pool_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pool_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            pool_info.queueFamilyIndex           = device->graphics_queue_family();
            VK_CHECK(vkCreateCommandPool(device->device(), &pool_info, nullptr, &command_pool));

            VkCommandBuffer             command_buffer = VK_NULL_HANDLE;
            VkCommandBufferAllocateInfo alloc_info     = {};
            alloc_info.sType                           = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool                     = command_pool;
            alloc_info.level                           = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount              = 1;
            VK_CHECK(vkAllocateCommandBuffers(device->device(), &alloc_info, &command_buffer));

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

            ImGui_ImplVulkan_CreateFontsTexture();

            VK_CHECK(vkEndCommandBuffer(command_buffer));

            VkSubmitInfo submit_info       = {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;
            VK_CHECK(vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE));
            VK_CHECK(vkQueueWaitIdle(device->graphics_queue()));

            vkFreeCommandBuffers(device->device(), command_pool, 1, &command_buffer);
            vkDestroyCommandPool(device->device(), command_pool, nullptr);
        }

        initialized_ = true;
        logger::info("ImGuiManager initialized");
    }

    void ImGuiManager::shutdown()
    {
        if (!initialized_ || !device_)
        {
            return;
        }

        vkDeviceWaitIdle(device_->device());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (descriptor_pool_ != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device_->device(), descriptor_pool_, nullptr);
            descriptor_pool_ = VK_NULL_HANDLE;
        }

        initialized_ = false;
        logger::info("ImGuiManager shutdown");
    }

    void ImGuiManager::begin_frame()
    {
        if (!initialized_)
        {
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw menu bar
        draw_menu_bar();
    }

    ImVec2 ImGuiManager::draw_editor_layout(rendering::SceneViewport* viewport)
    {
        ImVec2 viewport_size = ImVec2(1280, 720);

        // === Left Panel: Scene Hierarchy ===
        if (show_scene_hierarchy_)
        {
            ImGui::Begin("Scene Hierarchy", &show_scene_hierarchy_);
            draw_scene_hierarchy();
            ImGui::End();
        }

        // === Center: Scene Viewport ===
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Scene Viewport");

        viewport_focused_ = ImGui::IsWindowFocused();
        viewport_hovered_ = ImGui::IsWindowHovered();

        // Get available size for viewport
        viewport_size = ImGui::GetContentRegionAvail();

        // Display scene texture
        if (viewport && viewport_size.x > 0 && viewport_size.y > 0)
        {
            // Ensure viewport matches panel size
            VkExtent2D current_extent = viewport->extent();
            if (current_extent.width != static_cast<uint32_t>(viewport_size.x) ||
                current_extent.height != static_cast<uint32_t>(viewport_size.y))
            {
                viewport->resize(
                                 static_cast<uint32_t>(viewport_size.x),
                                 static_cast<uint32_t>(viewport_size.y));
            }

            // Show the rendered texture
            ImTextureID texture_id = viewport->imgui_texture_id();
            if (texture_id)
            {
                ImGui::Image(texture_id, viewport_size);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();

        // === Right Panel: Properties ===
        if (show_stats_panel_)
        {
            ImGui::Begin("Stats", &show_stats_panel_);
            draw_stats_panel();
            ImGui::End();
        }

        if (show_material_panel_)
        {
            ImGui::Begin("Material", &show_material_panel_);
            draw_material_panel();
            ImGui::End();
        }

        // === Demo Window (optional) ===
        if (show_demo_window_)
        {
            ImGui::ShowDemoWindow(&show_demo_window_);
        }

        return viewport_size;
    }

    void ImGuiManager::end_frame()
    {
        if (!initialized_)
        {
            return;
        }

        ImGui::Render();
    }

    void ImGuiManager::render(VkCommandBuffer command_buffer)
    {
        if (!initialized_)
        {
            return;
        }

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data)
        {
            ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
        }
    }

    void ImGuiManager::create_descriptor_pool(uint32_t image_count)
    {
        (void)image_count; // Not used in current implementation
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes                 = pool_sizes;

        VkResult result = vkCreateDescriptorPool(device_->device(), &pool_info, nullptr, &descriptor_pool_);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
    }

    void ImGuiManager::draw_menu_bar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))
                {
                }
                if (ImGui::MenuItem("Open..."))
                {
                }
                if (ImGui::MenuItem("Save"))
                {
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Scene Hierarchy", nullptr, &show_scene_hierarchy_);
                ImGui::MenuItem("Stats", nullptr, &show_stats_panel_);
                ImGui::MenuItem("Material", nullptr, &show_material_panel_);
                ImGui::Separator();
                ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Render"))
            {
                if (ImGui::MenuItem("Reload Shaders"))
                {
                }
                if (ImGui::MenuItem("Capture Frame"))
                {
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void ImGuiManager::draw_stats_panel()
    {
        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", stats_data_.fps);
        ImGui::Text("Frame Time: %.2f ms", stats_data_.frame_time);

        ImGui::Spacing();
        ImGui::Text("Scene");
        ImGui::Separator();
        ImGui::Text("Triangles: %u", stats_data_.triangle_count);
        ImGui::Text("Draw Calls: %u", stats_data_.draw_calls);

        ImGui::Spacing();
        ImGui::Text("Current Material: %s", stats_data_.current_material.c_str());
    }

    void ImGuiManager::draw_material_panel()
    {
        ImGui::Text("Material Properties");
        ImGui::Separator();

        static float roughness = 0.5f;
        static float metallic  = 0.0f;
        static float albedo[3] = {1.0f, 1.0f, 1.0f};

        ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
        ImGui::ColorEdit3("Albedo", albedo);

        if (ImGui::Button("Apply"))
        {
            // TODO: Apply material changes
        }
    }

    void ImGuiManager::draw_scene_hierarchy()
    {
        ImGui::Text("Scene Objects");
        ImGui::Separator();

        // Placeholder scene tree
        if (ImGui::TreeNode("Scene Root"))
        {
            if (ImGui::Selectable("Main Camera", true))
            {
                // Select camera
            }
            if (ImGui::Selectable("Directional Light"))
            {
                // Select light
            }
            if (ImGui::Selectable("Cube Mesh"))
            {
                // Select mesh
            }
            ImGui::TreePop();
        }
    }
} // namespace vulkan_engine::editor
