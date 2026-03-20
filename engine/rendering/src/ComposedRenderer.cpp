#include "engine/rendering/ComposedRenderer.hpp"
#include "engine/editor/Editor.hpp"
#include "engine/rhi/vulkan/device/Device.hpp"
#include "engine/rhi/vulkan/device/SwapChain.hpp"
#include "engine/platform/windowing/Window.hpp"
#include "engine/core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    ComposedRenderer::ComposedRenderer() = default;

    ComposedRenderer::~ComposedRenderer()
    {
        if (initialized_)
        {
            shutdown();
        }
    }

    ComposedRenderer::ComposedRenderer(ComposedRenderer&& other) noexcept
        : initialized_(other.initialized_)
        , scene_renderer_(std::move(other.scene_renderer_))
        , ui_renderer_(std::move(other.ui_renderer_))
        , current_frame_(other.current_frame_)
    {
        other.initialized_ = false;
    }

    ComposedRenderer& ComposedRenderer::operator=(ComposedRenderer&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            initialized_    = other.initialized_;
            scene_renderer_ = std::move(other.scene_renderer_);
            ui_renderer_    = std::move(other.ui_renderer_);
            current_frame_  = other.current_frame_;

            other.initialized_ = false;
        }
        return *this;
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool ComposedRenderer::initialize(
        std::shared_ptr<platform::Window>      window,
        std::shared_ptr<vulkan::DeviceManager> device,
        std::shared_ptr<vulkan::SwapChain>     swap_chain,
        const Config&                          config)
    {
        if (initialized_)
        {
            logger::warn("ComposedRenderer already initialized");
            return true;
        }

        logger::info("Initializing ComposedRenderer...");

        // 鍒濆鍖栧満鏅覆鏌撳櫒
        SceneRenderer::Config scene_config;
        scene_config.width                = config.scene_width;
        scene_config.height               = config.scene_height;
        scene_config.enable_gpu_timing    = config.enable_gpu_timing;
        scene_config.max_frames_in_flight = config.max_frames_in_flight;

        if (!scene_renderer_.initialize(device, scene_config))
        {
            logger::error("Failed to initialize SceneRenderer");
            return false;
        }

        // 鍒濆鍖朥I娓叉煋鍣?
        UIRenderer::Config ui_config;
        ui_config.max_frames_in_flight = config.max_frames_in_flight;
        ui_config.enable_vsync         = config.enable_vsync;

        if (!ui_renderer_.initialize(window, device, swap_chain, ui_config))
        {
            logger::error("Failed to initialize UIRenderer");
            scene_renderer_.shutdown();
            return false;
        }

        initialized_ = true;
        logger::info("ComposedRenderer initialized successfully");
        return true;
    }

    void ComposedRenderer::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        logger::info("Shutting down ComposedRenderer...");

        scene_renderer_.shutdown();
        ui_renderer_.shutdown();

        initialized_ = false;
        logger::info("ComposedRenderer shutdown complete");
    }

    // ============================================================================
    // Render Loop
    // ============================================================================

    bool ComposedRenderer::begin_frame()
    {
        if (!initialized_)
        {
            return false;
        }

        // 1. 寮€濮嬪満鏅抚锛堝満鏅覆鏌撳櫒鏈夎嚜宸辩殑鏆傚仠鎺у埗锛?
        scene_renderer_.begin_frame();

        // 2. 鑾峰彇UI鐨?swap chain image
        bool ui_ready = ui_renderer_.acquire_next_image();

        if (!ui_ready)
        {
            return false;
        }

        // 鍦烘櫙娓叉煋鍣ㄥ彲鑳芥殏鍋滐紝浣哢I蹇呴』缁х画
        return true;
    }

    void ComposedRenderer::render_scene(SceneRenderCallback callback)
    {
        if (!initialized_)
        {
            return;
        }

        // 鍦烘櫙娓叉煋鍣ㄦ湁鑷繁鐨勬殏鍋滄帶鍒?
        scene_renderer_.render(callback);
    }

    void ComposedRenderer::render_ui(editor::Editor& editor)
    {
        if (!initialized_)
        {
            return;
        }

        // 濡傛灉鍦烘櫙娌℃湁鏆傚仠锛屼紶閫掑満鏅畬鎴愮殑淇″彿閲忎緵UI绛夊緟
        VkSemaphore scene_semaphore = scene_renderer_.is_paused()
                                          ? VK_NULL_HANDLE
                                          : scene_renderer_.get_scene_finished_semaphore();

        // 浼犻€掑満鏅覆鏌撶洰鏍囦緵UI鏄剧ず
        ui_renderer_.render(editor, scene_renderer_.render_target(), scene_semaphore);
    }

    void ComposedRenderer::end_frame()
    {
        if (!initialized_)
        {
            return;
        }

        // 1. 缁撴潫鍦烘櫙甯э紙鎺ㄨ繘鍒颁笅涓€甯э級
        scene_renderer_.end_frame();

        // 2. 鍛堢幇UI
        ui_renderer_.present();

        current_frame_ = (current_frame_ + 1) % 2; // 鍋囪 max_frames_in_flight = 2
    }

    // ============================================================================
    // Resize
    // ============================================================================

    void ComposedRenderer::resize(uint32_t width, uint32_t height)
    {
        // 鍚屾椂璋冩暣鍦烘櫙鍜孶I
        resize_scene(width, height);
        resize_ui(width, height);
    }

    void ComposedRenderer::resize_scene(uint32_t width, uint32_t height)
    {
        scene_renderer_.resize(width, height);
    }

    void ComposedRenderer::resize_ui(uint32_t width, uint32_t height)
    {
        ui_renderer_.resize(width, height);
    }
} // namespace vulkan_engine::rendering
