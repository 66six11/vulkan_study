#include "rendering/SceneRenderer.hpp"
#include "rendering/Viewport.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/pipelines/RenderPassManager.hpp"
#include "vulkan/memory/VmaAllocator.hpp"
#include "vulkan/memory/VmaImage.hpp"
#include "core/utils/Logger.hpp"

#include <algorithm>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    SceneRenderer::SceneRenderer() = default;

    SceneRenderer::~SceneRenderer()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    SceneRenderer::SceneRenderer(SceneRenderer&& other) noexcept
        : config_(std::move(other.config_))
        , initialized_(other.initialized_)
        , paused_(other.paused_)
        , device_(std::move(other.device_))
        , vma_allocator_(std::move(other.vma_allocator_))
        , render_graph_(std::move(other.render_graph_))
        , render_target_(std::move(other.render_target_))
        , viewport_(std::move(other.viewport_))
        , render_pass_manager_(std::move(other.render_pass_manager_))
        , frame_syncs_(std::move(other.frame_syncs_))
        , command_pool_(std::move(other.command_pool_))
        , command_buffers_(std::move(other.command_buffers_))
        , query_pools_(std::move(other.query_pools_))
        , gpu_frame_times_(std::move(other.gpu_frame_times_))
        , gpu_time_write_index_(other.gpu_time_write_index_)
        , gpu_render_time_ms_(other.gpu_render_time_ms_)
        , query_pools_initialized_(std::move(other.query_pools_initialized_))
        , current_frame_(other.current_frame_)
        , frame_started_(other.frame_started_)
        , resize_pending_(other.resize_pending_)
        , pending_width_(other.pending_width_)
        , pending_height_(other.pending_height_)
    {
        other.initialized_   = false;
        other.frame_started_ = false;
    }

    SceneRenderer& SceneRenderer::operator=(SceneRenderer&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            config_                  = std::move(other.config_);
            initialized_             = other.initialized_;
            paused_                  = other.paused_;
            device_                  = std::move(other.device_);
            vma_allocator_           = std::move(other.vma_allocator_);
            render_graph_            = std::move(other.render_graph_);
            render_target_           = std::move(other.render_target_);
            viewport_                = std::move(other.viewport_);
            render_pass_manager_     = std::move(other.render_pass_manager_);
            frame_syncs_             = std::move(other.frame_syncs_);
            command_pool_            = std::move(other.command_pool_);
            command_buffers_         = std::move(other.command_buffers_);
            query_pools_             = std::move(other.query_pools_);
            gpu_frame_times_         = std::move(other.gpu_frame_times_);
            gpu_time_write_index_    = other.gpu_time_write_index_;
            gpu_render_time_ms_      = other.gpu_render_time_ms_;
            query_pools_initialized_ = std::move(other.query_pools_initialized_);
            current_frame_           = other.current_frame_;
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

    bool SceneRenderer::initialize(
        std::shared_ptr<vulkan::DeviceManager> device,
        const Config&                          config)
    {
        if (initialized_)
        {
            logger::warn("SceneRenderer already initialized");
            return true;
        }

        device_ = device;
        config_ = config;

        if (!device_)
        {
            logger::error("Device is null");
            return false;
        }

        logger::info("Initializing SceneRenderer...");

        if (!initialize_vma_allocator()) return false;
        if (!initialize_render_pass_manager()) return false;
        if (!initialize_frame_sync()) return false;
        if (!initialize_command_pool()) return false;
        if (!initialize_render_target()) return false;
        if (!initialize_viewport()) return false;
        if (!initialize_query_pools()) return false;

        render_graph_.initialize(device_);

        initialized_ = true;
        logger::info("SceneRenderer initialized successfully");
        return true;
    }

    bool SceneRenderer::initialize_vma_allocator()
    {
        vulkan::memory::VmaAllocator::CreateInfo allocator_info;
        allocator_info.enableBudget = true;
        vma_allocator_              = std::make_shared<vulkan::memory::VmaAllocator>(device_, allocator_info);
        logger::info("VMA Allocator created");
        return true;
    }

    bool SceneRenderer::initialize_render_pass_manager()
    {
        render_pass_manager_ = std::make_unique<vulkan::RenderPassManager>(device_);
        logger::info("RenderPassManager created");
        return true;
    }

    bool SceneRenderer::initialize_frame_sync()
    {
        frame_syncs_.resize(config_.max_frames_in_flight);

        for (uint32_t i = 0; i < config_.max_frames_in_flight; ++i)
        {
            frame_syncs_[i].in_flight_fence          = std::make_unique<vulkan::Fence>(device_, true); // signaled
            frame_syncs_[i].scene_finished_semaphore = std::make_unique<vulkan::Semaphore>(device_);
        }

        logger::info("Frame sync objects created: " + std::to_string(config_.max_frames_in_flight));
        return true;
    }

    bool SceneRenderer::initialize_command_pool()
    {
        command_pool_ = std::make_unique<vulkan::RenderCommandPool>(
                                                                    device_,
                                                                    0,
                                                                    // graphics queue family
                                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        command_buffers_ = command_pool_->allocate(config_.max_frames_in_flight);

        logger::info("Command pool initialized");
        return true;
    }

    bool SceneRenderer::initialize_query_pools()
    {
        if (!config_.enable_gpu_timing)
        {
            logger::info("GPU timing disabled");
            return true;
        }

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
            pool_info.queryCount = 2;

            if (vkCreateQueryPool(device_->device().handle(), &pool_info, nullptr, &query_pools_[i]) != VK_SUCCESS)
            {
                logger::error("Failed to create query pool for frame " + std::to_string(i));
                return false;
            }
        }

        // 初始化查询池
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool        = command_pool_->handle();
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

            vkFreeCommandBuffers(device_->device().handle(), command_pool_->handle(), 1, &init_cmd);

            std::fill(query_pools_initialized_.begin(), query_pools_initialized_.end(), true);
        }

        logger::info("GPU query pools created");
        return true;
    }

    bool SceneRenderer::initialize_render_target()
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

        auto offscreen_pass = render_pass_manager_->get_offscreen_render_pass(
                                                                              render_target_->color_format(),
                                                                              render_target_->depth_format());

        render_target_->create_framebuffer(offscreen_pass);

        logger::info("RenderTarget initialized: " + std::to_string(config_.width) + "x" + std::to_string(config_.height));
        return true;
    }

    bool SceneRenderer::initialize_viewport()
    {
        viewport_ = std::make_shared<Viewport>();
        viewport_->initialize(device_, render_target_);
        logger::info("Viewport initialized");
        return true;
    }

    void SceneRenderer::shutdown()
    {
        if (!initialized_ || !device_)
        {
            return;
        }

        logger::info("Shutting down SceneRenderer...");

        vkDeviceWaitIdle(device_->device().handle());
        cleanup_resources();

        initialized_ = false;
        logger::info("SceneRenderer shutdown complete");
    }

    void SceneRenderer::cleanup_resources()
    {
        render_graph_.reset();
        destroy_query_pools();

        viewport_.reset();
        render_target_.reset();

        command_buffers_.clear();
        command_pool_.reset();

        frame_syncs_.clear();

        render_pass_manager_.reset();
        vma_allocator_.reset();
    }

    void SceneRenderer::destroy_query_pools()
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

    bool SceneRenderer::begin_frame()
    {
        if (!initialized_ || paused_)
        {
            return false;
        }

        // 等待上一帧完成
        auto& sync = frame_syncs_[current_frame_];
        sync.in_flight_fence->wait();
        sync.in_flight_fence->reset();

        // Explicitly reset command buffer after fence wait to ensure it's not in use
        if (current_frame_ < command_buffers_.size())
        {
            vkResetCommandBuffer(command_buffers_[current_frame_].handle(), 0);
        }

        // 更新 GPU 计时
        if (config_.enable_gpu_timing && !query_pools_.empty())
        {
            update_gpu_timing();
        }

        frame_started_ = true;
        return true;
    }

    void SceneRenderer::render(SceneRenderCallback callback)
    {
        if (!frame_started_ || !callback)
        {
            return;
        }

        record_commands(callback);
        submit_commands();
    }

    void SceneRenderer::record_commands(SceneRenderCallback callback)
    {
        auto&           cmd        = command_buffers_[current_frame_];
        VkCommandBuffer cmd_handle = cmd.handle();

        if (!render_target_ || render_target_->width() < 10 || render_target_->height() < 10)
        {
            logger::warn("RenderTarget too small, skipping scene render");
            return;
        }

        vkResetCommandBuffer(cmd_handle, 0);
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // 写入开始时间戳
        if (!query_pools_.empty() && query_pools_[current_frame_] != VK_NULL_HANDLE)
        {
            vkCmdResetQueryPool(cmd_handle, query_pools_[current_frame_], 0, 2);
            vkCmdWriteTimestamp(cmd_handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pools_[current_frame_], 0);
        }

        // Dynamic Rendering
        VkImageView color_view = render_target_->color_image_view();
        VkImageView depth_view = render_target_->depth_image_view();

        if (color_view != VK_NULL_HANDLE && depth_view != VK_NULL_HANDLE)
        {
            // 将 color image 转换为 COLOR_ATTACHMENT_OPTIMAL
            // 注意：第一帧时图像可能是 COLOR_ATTACHMENT_OPTIMAL（初始状态）
            // 后续帧是 SHADER_READ_ONLY_OPTIMAL（上一帧结束时转换）
            // 使用 UNDEFINED 作为 oldLayout 可以处理两种情况
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED; // 兼容初始状态和后续帧
            barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = render_target_->color_image()->handle();
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = 0; // UNDEFINED 不需要等待之前的访问
            barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                                 cmd_handle,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 // 最早阶段
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

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

            FrameContext ctx{};
            ctx.frame_index  = current_frame_;
            ctx.width        = render_target_->width();
            ctx.height       = render_target_->height();
            ctx.delta_time   = 0.0f;
            ctx.elapsed_time = 0.0f;

            callback(cmd, ctx);

            cmd.end_dynamic_rendering();
        }

        // 转换 color image 到 SHADER_READ_ONLY_OPTIMAL
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

        // 写入结束时间戳
        if (!query_pools_.empty() && query_pools_[current_frame_] != VK_NULL_HANDLE)
        {
            vkCmdWriteTimestamp(cmd_handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pools_[current_frame_], 1);
        }

        cmd.end();
    }

    void SceneRenderer::submit_commands()
    {
        auto& cmd  = command_buffers_[current_frame_];
        auto& sync = frame_syncs_[current_frame_];

        VkSemaphore signal_semaphores[] = {sync.scene_finished_semaphore->handle()};

        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount   = 1;
        VkCommandBuffer cmd_handle       = cmd.handle();
        submit_info.pCommandBuffers      = &cmd_handle;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        vkQueueSubmit(device_->graphics_queue().handle(), 1, &submit_info, sync.in_flight_fence->handle());
    }

    void SceneRenderer::end_frame()
    {
        if (!frame_started_)
        {
            return;
        }

        current_frame_ = (current_frame_ + 1) % config_.max_frames_in_flight;
        frame_started_ = false;

        if (resize_pending_)
        {
            apply_pending_resize();
        }
    }

    // ============================================================================
    // GPU Timing
    // ============================================================================

    void SceneRenderer::update_gpu_timing()
    {
        if (query_pools_.empty() || current_frame_ >= query_pools_.size())
            return;

        uint32_t prev_frame = (current_frame_ + config_.max_frames_in_flight - 1) % config_.max_frames_in_flight;

        if (!query_pools_initialized_[prev_frame])
            return;

        uint64_t timestamps[2];
        VkResult result = vkGetQueryPoolResults(
                                                device_->device().handle(),
                                                query_pools_[prev_frame],
                                                0,
                                                2,
                                                sizeof(timestamps),
                                                timestamps,
                                                sizeof(uint64_t),
                                                VK_QUERY_RESULT_64_BIT);

        if (result == VK_SUCCESS)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device_->physical_device().handle(), &props);
            float timestamp_period = props.limits.timestampPeriod;

            float frame_time_ns = static_cast<float>(timestamps[1] - timestamps[0]) * timestamp_period;
            float frame_time_ms = frame_time_ns / 1000000.0f;

            gpu_frame_times_[gpu_time_write_index_] = frame_time_ms;
            gpu_time_write_index_                   = (gpu_time_write_index_ + 1) % GPU_TIME_HISTORY_SIZE;

            float total_time = 0.0f;
            for (float time : gpu_frame_times_)
            {
                total_time += time;
            }
            gpu_render_time_ms_ = total_time / GPU_TIME_HISTORY_SIZE;
        }
    }

    // ============================================================================
    // Resize
    // ============================================================================

    void SceneRenderer::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        resize_pending_ = true;
        pending_width_  = width;
        pending_height_ = height;

        config_.width  = width;
        config_.height = height;

        logger::info("SceneRenderer resize pending: " + std::to_string(width) + "x" + std::to_string(height));
    }

    void SceneRenderer::apply_pending_resize()
    {
        if (!resize_pending_)
        {
            return;
        }

        logger::info("Applying SceneRenderer resize: " + std::to_string(pending_width_) + "x" + std::to_string(pending_height_));

        vkDeviceWaitIdle(device_->device().handle());

        recreate_render_target();

        if (viewport_)
        {
            viewport_->resize(pending_width_, pending_height_);
        }

        resize_pending_ = false;
    }

    void SceneRenderer::recreate_render_target()
    {
        if (!render_target_)
        {
            return;
        }

        render_target_->resize(pending_width_, pending_height_);

        auto offscreen_pass = render_pass_manager_->get_offscreen_render_pass(
                                                                              render_target_->color_format(),
                                                                              render_target_->depth_format());

        render_target_->create_framebuffer(offscreen_pass);

        logger::info("RenderTarget recreated");
    }

    // ============================================================================
    // Getters
    // ============================================================================

    VkImageView SceneRenderer::get_color_image_view() const
    {
        return render_target_ ? render_target_->color_image_view() : VK_NULL_HANDLE;
    }

    VkImageView SceneRenderer::get_depth_image_view() const
    {
        return render_target_ ? render_target_->depth_image_view() : VK_NULL_HANDLE;
    }

    VkSemaphore SceneRenderer::get_scene_finished_semaphore() const
    {
        if (current_frame_ < frame_syncs_.size() && frame_syncs_[current_frame_].scene_finished_semaphore)
        {
            return frame_syncs_[current_frame_].scene_finished_semaphore->handle();
        }
        return VK_NULL_HANDLE;
    }

    void SceneRenderer::compile_render_graph()
    {
        render_graph_.compile();
    }

    void SceneRenderer::reset_render_graph()
    {
        render_graph_.reset();
    }
} // namespace vulkan_engine::rendering
