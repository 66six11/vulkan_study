#include "engine/rendering/Renderer.hpp"
#include "engine/rendering/Viewport.hpp"
#include "engine/rendering/render_graph/RenderGraphPass.hpp"
#include "engine/rhi/vulkan/device/SwapChain.hpp"
#include "engine/rhi/vulkan/resources/DepthBuffer.hpp"
#include "engine/rhi/vulkan/pipelines/RenderPassManager.hpp"
#include "engine/rhi/vulkan/resources/Framebuffer.hpp"
#include "engine/rhi/vulkan/memory/VmaAllocator.hpp"
#include "engine/rhi/vulkan/memory/VmaImage.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/editor/Editor.hpp"

#include <algorithm>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    Renderer::Renderer() = default;

    Renderer::~Renderer()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    Renderer::Renderer(Renderer&& other) noexcept
        : config_(std::move(other.config_))
        , initialized_(other.initialized_)
        , device_(std::move(other.device_))
        , swap_chain_(std::move(other.swap_chain_))
        , vma_allocator_(std::move(other.vma_allocator_))
        , render_graph_(std::move(other.render_graph_))
        , render_target_(std::move(other.render_target_))
        , viewport_(std::move(other.viewport_))
        , depth_buffer_(std::move(other.depth_buffer_))
        , render_pass_manager_(std::move(other.render_pass_manager_))
        , frame_sync_(std::move(other.frame_sync_))
        , scene_cmd_pool_(std::move(other.scene_cmd_pool_))
        , ui_cmd_pool_(std::move(other.ui_cmd_pool_))
        , scene_cmd_buffers_(std::move(other.scene_cmd_buffers_))
        , ui_cmd_buffers_(std::move(other.ui_cmd_buffers_))
        , framebuffer_pool_(std::move(other.framebuffer_pool_))
        , query_pools_(std::move(other.query_pools_))
        , gpu_frame_times_(std::move(other.gpu_frame_times_))
        , gpu_time_write_index_(other.gpu_time_write_index_)
        , gpu_render_time_ms_(other.gpu_render_time_ms_)
        , query_pools_initialized_(std::move(other.query_pools_initialized_))
        , current_frame_(other.current_frame_)
        , current_image_(other.current_image_)
        , frame_started_(other.frame_started_)
        , resize_pending_(other.resize_pending_)
        , pending_width_(other.pending_width_)
        , pending_height_(other.pending_height_)
    {
        other.initialized_   = false;
        other.frame_started_ = false;
    }

    Renderer& Renderer::operator=(Renderer&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            config_                  = std::move(other.config_);
            initialized_             = other.initialized_;
            device_                  = std::move(other.device_);
            swap_chain_              = std::move(other.swap_chain_);
            vma_allocator_           = std::move(other.vma_allocator_);
            render_graph_            = std::move(other.render_graph_);
            render_target_           = std::move(other.render_target_);
            viewport_                = std::move(other.viewport_);
            depth_buffer_            = std::move(other.depth_buffer_);
            render_pass_manager_     = std::move(other.render_pass_manager_);
            frame_sync_              = std::move(other.frame_sync_);
            scene_cmd_pool_          = std::move(other.scene_cmd_pool_);
            ui_cmd_pool_             = std::move(other.ui_cmd_pool_);
            scene_cmd_buffers_       = std::move(other.scene_cmd_buffers_);
            ui_cmd_buffers_          = std::move(other.ui_cmd_buffers_);
            framebuffer_pool_        = std::move(other.framebuffer_pool_);
            query_pools_             = std::move(other.query_pools_);
            gpu_frame_times_         = std::move(other.gpu_frame_times_);
            gpu_time_write_index_    = other.gpu_time_write_index_;
            gpu_render_time_ms_      = other.gpu_render_time_ms_;
            query_pools_initialized_ = std::move(other.query_pools_initialized_);
            current_frame_           = other.current_frame_;
            current_image_           = other.current_image_;
            frame_started_           = other.frame_started_;
            resize_pending_          = other.resize_pending_;
            pending_width_           = other.pending_width_;
            pending_height_          = other.pending_height_;

            other.initialized_   = false;
            other.frame_started_ = false;
        }
        return *this;
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool Renderer::initialize(
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<vulkan::SwapChain>     swap_chain,
        const Config&                          config)
    {
        if (initialized_)
        {
            logger::warn("Renderer already initialized");
            return true;
        }

        device_     = device;
        swap_chain_ = swap_chain;
        config_     = config;

        if (!device_ || !swap_chain_)
        {
            logger::error("Device or swap chain is null");
            return false;
        }

        logger::info("Initializing Renderer...");

        // 鍒濆鍖栧瓙绯荤粺锛堥『搴忓緢閲嶈锛?
        if (!initialize_vma_allocator()) return false;
        if (!initialize_depth_buffer()) return false;
        if (!initialize_render_pass_manager()) return false;
        if (!initialize_frame_sync()) return false;
        if (!initialize_command_pools()) return false;
        if (!initialize_framebuffer_pool()) return false;
        if (!initialize_render_target()) return false;
        if (!initialize_viewport()) return false;
        if (!initialize_query_pools()) return false;

        // 鍒濆鍖?RenderGraph
        render_graph_.initialize(device_);

        initialized_ = true;
        logger::info("Renderer initialized successfully");
        return true;
    }

    bool Renderer::initialize_vma_allocator()
    {
        vulkan::memory::VmaAllocator::CreateInfo allocator_info;
        allocator_info.enableBudget = true;
        vma_allocator_              = std::make_shared<vulkan::memory::VmaAllocator>(device_, allocator_info);
        logger::info("VMA Allocator created");
        return true;
    }

    bool Renderer::initialize_depth_buffer()
    {
        depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(
                                                              device_,
                                                              swap_chain_->width(),
                                                              swap_chain_->height());
        logger::info("Depth buffer created: " + std::to_string(swap_chain_->width()) + "x" + std::to_string(swap_chain_->height()));
        return true;
    }

    bool Renderer::initialize_render_pass_manager()
    {
        render_pass_manager_ = std::make_unique<vulkan::RenderPassManager>(device_);
        logger::info("RenderPassManager created");
        return true;
    }

    bool Renderer::initialize_frame_sync()
    {
        frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device_, config_.max_frames_in_flight);
        frame_sync_->resize_render_finished_semaphores(swap_chain_->image_count());
        logger::info("FrameSyncManager created with " + std::to_string(config_.max_frames_in_flight) + " frames");
        return true;
    }

    bool Renderer::initialize_command_pools()
    {
        // 鍦烘櫙娓叉煋鍛戒护姹?
        scene_cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                      device_,
                                                                      0,
                                                                      // graphics queue family
                                                                      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // UI 娓叉煋鍛戒护姹?
        ui_cmd_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                   device_,
                                                                   0,
                                                                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // 鍒嗛厤鍛戒护缂撳啿
        scene_cmd_buffers_ = scene_cmd_pool_->allocate(config_.max_frames_in_flight);
        ui_cmd_buffers_    = ui_cmd_pool_->allocate(config_.max_frames_in_flight);

        logger::info("Command pools initialized");
        return true;
    }

    bool Renderer::initialize_query_pools()
    {
        if (!config_.enable_gpu_timing)
        {
            logger::info("GPU timing disabled");
            return true;
        }

        // 妫€鏌ヨ澶囨槸鍚︽敮鎸佹椂闂存埑鏌ヨ
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device_->physical_device().handle(), &props);
        if (props.limits.timestampComputeAndGraphics == VK_FALSE)
        {
            logger::warn("GPU timestamp queries not supported");
            return true;
        }

        query_pools_.resize(config_.max_frames_in_flight);
        query_pools_initialized_.resize(config_.max_frames_in_flight, false);
        gpu_frame_times_.resize(GPU_TIME_HISTORY_SIZE, 0.0f);

        for (uint32_t i = 0; i < config_.max_frames_in_flight; ++i)
        {
            VkQueryPoolCreateInfo pool_info{};
            pool_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            pool_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
            pool_info.queryCount = 2; // Start and end timestamps

            if (vkCreateQueryPool(device_->device().handle(), &pool_info, nullptr, &query_pools_[i]) != VK_SUCCESS)
            {
                logger::error("Failed to create query pool for frame " + std::to_string(i));
                return false;
            }
        }

        // 鍒濆鍖栨煡璇㈡睜锛堥噸缃墍鏈夋煡璇級
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool        = scene_cmd_pool_->handle();
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandBufferCount = 1;

            VkCommandBuffer init_cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(device_->device().handle(), &alloc_info, &init_cmd);

            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(init_cmd, &begin_info);

            for (size_t i = 0; i < query_pools_.size(); ++i)
            {
                vkCmdResetQueryPool(init_cmd, query_pools_[i], 0, 2);
            }

            vkEndCommandBuffer(init_cmd);

            VkSubmitInfo submit_info{};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &init_cmd;

            vkQueueSubmit(device_->graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(device_->graphics_queue().handle());

            vkFreeCommandBuffers(device_->device().handle(), scene_cmd_pool_->handle(), 1, &init_cmd);

            std::fill(query_pools_initialized_.begin(), query_pools_initialized_.end(), true);
        }

        logger::info("GPU query pools created: " + std::to_string(config_.max_frames_in_flight));
        return true;
    }

    bool Renderer::initialize_framebuffer_pool()
    {
        framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device_);

        // 涓?swap chain images 鍒涘缓 framebuffer
        std::vector<VkImageView> image_views;
        image_views.reserve(swap_chain_->image_count());
        for (uint32_t i = 0; i < swap_chain_->image_count(); ++i)
        {
            image_views.push_back(swap_chain_->get_image(i).view);
        }

        auto render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                                                                                    swap_chain_->format(),
                                                                                    depth_buffer_->format());

        framebuffer_pool_->create_for_swap_chain(
                                                 render_pass,
                                                 image_views,
                                                 swap_chain_->width(),
                                                 swap_chain_->height(),
                                                 depth_buffer_->view());

        logger::info("Framebuffer pool initialized with " + std::to_string(swap_chain_->image_count()) + " framebuffers");
        return true;
    }

    bool Renderer::initialize_render_target()
    {
        RenderTarget::CreateInfo rt_info;
        rt_info.width        = config_.width;
        rt_info.height       = config_.height;
        rt_info.color_format = VK_FORMAT_B8G8R8A8_UNORM;
        rt_info.depth_format = VK_FORMAT_D32_SFLOAT;
        rt_info.create_color = true;
        rt_info.create_depth = true;

        render_target_ = std::make_shared<RenderTarget>();
        render_target_->initialize(vma_allocator_, rt_info);

        // 鍒涘缓绂诲睆 RenderPass
        auto offscreen_pass = render_pass_manager_->get_offscreen_render_pass(
                                                                              render_target_->color_format(),
                                                                              render_target_->depth_format());

        // 鍒涘缓 Framebuffer
        render_target_->create_framebuffer(offscreen_pass);

        logger::info("RenderTarget initialized: " + std::to_string(config_.width) + "x" + std::to_string(config_.height));
        return true;
    }

    bool Renderer::initialize_viewport()
    {
        viewport_ = std::make_shared<Viewport>();
        viewport_->initialize(device_, render_target_);
        logger::info("Viewport initialized");
        return true;
    }

    void Renderer::shutdown()
    {
        if (!initialized_ || !device_)
        {
            return;
        }

        logger::info("Shutting down Renderer...");

        // 绛夊緟璁惧绌洪棽
        vkDeviceWaitIdle(device_->device().handle());

        cleanup_resources();

        initialized_ = false;
        logger::info("Renderer shutdown complete");
    }

    void Renderer::cleanup_resources()
    {
        // 1. 娓呯悊 RenderGraph
        render_graph_.reset();

        // 2. 娓呯悊 query pools
        destroy_query_pools();

        // 3. 娓呯悊 viewport 鍜?render target
        viewport_.reset();
        render_target_.reset();

        // 4. 娓呯悊鍛戒护缂撳啿鍜屾睜
        scene_cmd_buffers_.clear();
        ui_cmd_buffers_.clear();
        scene_cmd_pool_.reset();
        ui_cmd_pool_.reset();

        // 5. 娓呯悊 framebuffer pool
        framebuffer_pool_.reset();

        // 6. 娓呯悊鍚屾瀵硅薄
        frame_sync_.reset();

        // 7. 娓呯悊娣卞害缂撳啿
        depth_buffer_.reset();

        // 8. 娓呯悊 render pass manager
        render_pass_manager_.reset();

        // 9. 娓呯悊 VMA 鍒嗛厤鍣?
        vma_allocator_.reset();
    }

    void Renderer::destroy_query_pools()
    {
        for (auto& pool : query_pools_)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyQueryPool(device_->device().handle(), pool, nullptr);
                pool = VK_NULL_HANDLE;
            }
        }
        query_pools_.clear();
        query_pools_initialized_.clear();
    }

    // ============================================================================
    // Render Loop
    // ============================================================================

    bool Renderer::begin_frame()
    {
        if (!initialized_ || !frame_sync_ || !swap_chain_)
        {
            return false;
        }

        // 绛夊緟涓婁竴甯у畬鎴愶紙CPU-GPU 鍚屾锛?
        frame_sync_->wait_and_reset_current_frame_fence();

        // 鑾峰彇涓嬩竴甯?image
        bool acquired = swap_chain_->acquire_next_image(
                                                        frame_sync_->get_current_acquire_semaphore().handle(),
                                                        VK_NULL_HANDLE,
                                                        current_image_);

        if (!acquired)
        {
            return false;
        }

        current_frame_ = frame_sync_->current_frame();
        frame_started_ = true;

        // 鏇存柊 GPU 璁℃椂锛堣鍙栦笂涓€甯х粨鏋滐級
        if (config_.enable_gpu_timing && !query_pools_.empty())
        {
            update_gpu_timing();
        }

        return true;
    }

    void Renderer::render_scene(SceneRenderCallback callback)
    {
        if (!frame_started_ || !callback)
        {
            return;
        }

        record_scene_commands(callback);
        submit_scene_commands();
    }

    void Renderer::record_scene_commands(SceneRenderCallback callback)
    {
        auto&           cmd        = scene_cmd_buffers_[current_frame_];
        VkCommandBuffer cmd_handle = cmd.handle();

        // 妫€鏌?RenderTarget 鏄惁鏈夋晥
        if (!render_target_ || render_target_->width() < 10 || render_target_->height() < 10)
        {
            logger::warn("RenderTarget too small, skipping scene render");
            return;
        }

        // 閲嶇疆骞跺綍鍒跺懡浠ょ紦鍐?
        vkResetCommandBuffer(cmd_handle, 0);
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // 鍐欏叆寮€濮嬫椂闂存埑
        if (!query_pools_.empty() && query_pools_[current_frame_] != VK_NULL_HANDLE)
        {
            vkCmdResetQueryPool(cmd_handle, query_pools_[current_frame_], 0, 2);
            vkCmdWriteTimestamp(cmd_handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pools_[current_frame_], 0);
        }

        // 浣跨敤 Dynamic Rendering 鎵ц鍦烘櫙娓叉煋
        VkImageView color_view = render_target_->color_image_view();
        VkImageView depth_view = render_target_->depth_image_view();

        if (color_view != VK_NULL_HANDLE && depth_view != VK_NULL_HANDLE)
        {
            VkClearValue color_clear{};
            color_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            VkClearValue depth_clear{};
            depth_clear.depthStencil = {1.0f, 0};

            cmd.begin_dynamic_rendering(
                                        color_view,
                                        depth_view,
                                        render_target_->width(),
                                        render_target_->height(),
                                        &color_clear,
                                        &depth_clear);

            // 鎵ц鍦烘櫙娓叉煋鍥炶皟锛堝寘鍚?RenderGraph 鎵ц锛?
            FrameContext ctx{};
            ctx.frame_index  = current_frame_;
            ctx.image_index  = current_image_;
            ctx.width        = render_target_->width();
            ctx.height       = render_target_->height();
            ctx.delta_time   = 0.0f;
            ctx.elapsed_time = 0.0f;

            callback(cmd, ctx);

            cmd.end_dynamic_rendering();
        }

        // 杞崲 RenderTarget color image 鍒?SHADER_READ_ONLY_OPTIMAL锛屼緵 ImGui 璇诲彇
        if (render_target_ && render_target_->color_image())
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = render_target_->color_image()->handle();
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                                 cmd_handle,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);
        }

        // 鍐欏叆缁撴潫鏃堕棿鎴?
        if (!query_pools_.empty() && query_pools_[current_frame_] != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(cmd_handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pools_[current_frame_], 1);
        }

        cmd.end();
    }

    void Renderer::submit_scene_commands()
    {
        auto& cmd = scene_cmd_buffers_[current_frame_];

        // 鎻愪氦鍦烘櫙娓叉煋锛屼俊鍙?scene_finished semaphore
        VkSemaphore signal_semaphores[] = {frame_sync_->get_current_scene_finished_semaphore().handle()};

        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount   = 1;
        VkCommandBuffer cmd_handle       = cmd.handle();
        submit_info.pCommandBuffers      = &cmd_handle;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        // 鍦烘櫙鎻愪氦涓嶉渶瑕?fence锛孖mGui 鎻愪氦浼氫娇鐢?frame_fence
        vkQueueSubmit(device_->graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE);
    }

    void Renderer::render_ui(editor::Editor& editor)
    {
        if (!frame_started_)
        {
            return;
        }

        // 浣跨敤 Dynamic Rendering 娓叉煋 UI 鍒?SwapChain
        record_ui_commands_dynamic(editor);
        submit_ui_commands_dynamic();
    }

    void Renderer::record_ui_commands_dynamic(editor::Editor& editor)
    {
        auto&           cmd        = ui_cmd_buffers_[current_frame_];
        VkCommandBuffer cmd_handle = cmd.handle();

        // 閲嶇疆骞跺綍鍒?
        vkResetCommandBuffer(cmd_handle, 0);
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // 杞崲 swap chain image 鍒?COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = swap_chain_->get_image(current_image_).image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
                             cmd_handle,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        // 寮€濮?Dynamic Rendering
        VkRenderingAttachmentInfo color_attachment{};
        color_attachment.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment.imageView        = swap_chain_->get_image(current_image_).view;
        color_attachment.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.clearValue.color = {0.2f, 0.2f, 0.2f, 1.0f};

        VkRenderingInfo rendering_info{};
        rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.renderArea           = {{0, 0}, {swap_chain_->width(), swap_chain_->height()}};
        rendering_info.layerCount           = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments    = &color_attachment;

        vkCmdBeginRendering(cmd_handle, &rendering_info);

        // 娓叉煋 ImGui
        editor.render_to_command_buffer(cmd_handle);

        vkCmdEndRendering(cmd_handle);

        // 杞崲 swap chain image 鍒?PRESENT_SRC_KHR
        barrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
                             cmd_handle,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        cmd.end();
    }

    void Renderer::submit_ui_commands_dynamic()
    {
        auto& cmd = ui_cmd_buffers_[current_frame_];

        // 绛夊緟锛?) swap chain image 鍙敤, 2) 鍦烘櫙娓叉煋瀹屾垚
        VkSemaphore wait_semaphores[] = {
            frame_sync_->get_current_acquire_semaphore().handle(),
            frame_sync_->get_current_scene_finished_semaphore().handle()
        };
        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        };

        // 淇″彿锛歳ender finished锛坧er-image锛?
        VkSemaphore signal_semaphores[] = {frame_sync_->get_render_finished_semaphore(current_image_).handle()};

        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount   = 2;
        submit_info.pWaitSemaphores      = wait_semaphores;
        submit_info.pWaitDstStageMask    = wait_stages;
        submit_info.commandBufferCount   = 1;
        VkCommandBuffer cmd_handle       = cmd.handle();
        submit_info.pCommandBuffers      = &cmd_handle;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        // 浣跨敤 frame fence
        VkFence frame_fence = frame_sync_->get_current_frame_fence().handle();
        vkQueueSubmit(device_->graphics_queue().handle(), 1, &submit_info, frame_fence);
    }

    void Renderer::end_frame()
    {
        if (!frame_started_)
        {
            return;
        }

        // Present
        VkSemaphore present_semaphore = frame_sync_->get_render_finished_semaphore(current_image_).handle();
        swap_chain_->present(device_->graphics_queue().handle(), current_image_, present_semaphore);

        // 鎺ㄨ繘鍒颁笅涓€甯?
        frame_sync_->advance_frame();
        frame_started_ = false;

        // 搴旂敤寰呭鐞嗙殑 resize锛堝湪甯ц竟鐣岋級
        if (resize_pending_)
        {
            apply_pending_resize();
        }
    }

    // ============================================================================
    // RenderGraph
    // ============================================================================

    void Renderer::compile_render_graph()
    {
        render_graph_.compile();
    }

    void Renderer::reset_render_graph()
    {
        render_graph_.reset();
    }

    // ============================================================================
    // Resize Handling
    // ============================================================================

    void Renderer::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        // 鏍囪涓哄緟澶勭悊锛屽湪甯ц竟鐣屽簲鐢?
        resize_pending_ = true;
        pending_width_  = width;
        pending_height_ = height;

        config_.width  = width;
        config_.height = height;

        logger::info("Renderer resize marked as pending: " + std::to_string(width) + "x" + std::to_string(height));
    }

    VkRenderPass Renderer::get_offscreen_render_pass() const
    {
        if (!render_pass_manager_ || !render_target_)
        {
            return VK_NULL_HANDLE;
        }
        return render_pass_manager_->get_offscreen_render_pass(
                                                               render_target_->color_format(),
                                                               render_target_->depth_format());
    }

    VkRenderPass Renderer::get_present_render_pass() const
    {
        if (!render_pass_manager_ || !swap_chain_ || !depth_buffer_)
        {
            return VK_NULL_HANDLE;
        }
        return render_pass_manager_->get_present_render_pass_with_depth(
                                                                        swap_chain_->format(),
                                                                        depth_buffer_->format());
    }

    void Renderer::apply_pending_resize()
    {
        if (!resize_pending_ || !device_ || !swap_chain_)
        {
            return;
        }

        resize_pending_ = false;

        // 绛夊緟 GPU 瀹屾垚
        vkDeviceWaitIdle(device_->device().handle());

        // 閲嶅缓 swap chain
        if (!swap_chain_->recreate())
        {
            logger::error("Failed to recreate swap chain");
            return;
        }

        // 閲嶅缓鐩稿叧璧勬簮
        recreate_swap_chain_resources();

        logger::info("Renderer resize applied: " + std::to_string(pending_width_) + "x" + std::to_string(pending_height_));
    }

    void Renderer::recreate_swap_chain_resources()
    {
        // 閲嶅缓娣卞害缂撳啿
        depth_buffer_.reset();
        depth_buffer_ = std::make_unique<vulkan::DepthBuffer>(
                                                              device_,
                                                              swap_chain_->width(),
                                                              swap_chain_->height());

        // 閲嶅缓 framebuffers
        framebuffer_pool_.reset();
        framebuffer_pool_ = std::make_unique<vulkan::FramebufferPool>(device_);

        std::vector<VkImageView> image_views;
        image_views.reserve(swap_chain_->image_count());
        for (uint32_t i = 0; i < swap_chain_->image_count(); ++i)
        {
            image_views.push_back(swap_chain_->get_image(i).view);
        }

        auto render_pass = render_pass_manager_->get_present_render_pass_with_depth(
                                                                                    swap_chain_->format(),
                                                                                    depth_buffer_->format());

        framebuffer_pool_->create_for_swap_chain(
                                                 render_pass,
                                                 image_views,
                                                 swap_chain_->width(),
                                                 swap_chain_->height(),
                                                 depth_buffer_->view());

        // 閲嶅缓鍚屾瀵硅薄
        frame_sync_.reset();
        frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device_, config_.max_frames_in_flight);
        frame_sync_->resize_render_finished_semaphores(swap_chain_->image_count());

        // 閲嶅缓 RenderTarget
        recreate_render_target();
    }

    void Renderer::recreate_render_target()
    {
        if (!render_target_)
        {
            return;
        }

        render_target_->resize(swap_chain_->width(), swap_chain_->height());

        // 閲嶆柊鍒涘缓 framebuffer
        auto offscreen_pass = render_pass_manager_->get_offscreen_render_pass(
                                                                              render_target_->color_format(),
                                                                              render_target_->depth_format());

        render_target_->create_framebuffer(offscreen_pass);

        // 鏇存柊 viewport
        if (viewport_)
        {
            viewport_->resize(swap_chain_->width(), swap_chain_->height());
        }
    }

    // ============================================================================
    // GPU Timing
    // ============================================================================

    void Renderer::update_gpu_timing()
    {
        if (query_pools_.empty() || current_frame_ >= query_pools_.size())
        {
            return;
        }

        // 璇诲彇涓婁竴甯х殑缁撴灉
        uint32_t prev_frame = (current_frame_ + 1) % config_.max_frames_in_flight;
        if (!query_pools_initialized_[prev_frame])
        {
            return;
        }

        uint64_t timestamps[2] = {0, 0};
        VkResult result        = vkGetQueryPoolResults(
                                                       device_->device().handle(),
                                                       query_pools_[prev_frame],
                                                       0,
                                                       2,
                                                       sizeof(timestamps),
                                                       timestamps,
                                                       sizeof(uint64_t),
                                                       VK_QUERY_RESULT_64_BIT);

        if (result == VK_SUCCESS && timestamps[1] > timestamps[0])
        {
            // 璁＄畻 GPU 鏃堕棿
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device_->physical_device().handle(), &props);
            float timestamp_period = props.limits.timestampPeriod; // nanoseconds per tick

            uint64_t gpu_time_ns = static_cast<uint64_t>((timestamps[1] - timestamps[0]) * timestamp_period);
            float    gpu_time_us = static_cast<float>(gpu_time_ns) / 1000.0f;

            // 瀛樺叆鐜舰缂撳啿鍖?
            gpu_frame_times_[gpu_time_write_index_] = gpu_time_us;
            gpu_time_write_index_                   = (gpu_time_write_index_ + 1) % GPU_TIME_HISTORY_SIZE;

            // 璁＄畻骞虫粦骞冲潎鍊?
            float sum = 0.0f;
            for (float t : gpu_frame_times_)
            {
                sum += t;
            }
            gpu_render_time_ms_ = (sum / GPU_TIME_HISTORY_SIZE) / 1000.0f; // 杞崲涓烘绉?
        }
    }
} // namespace vulkan_engine::rendering
