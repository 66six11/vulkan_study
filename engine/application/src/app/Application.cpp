#include "engine/application/app/Application.hpp"
#include "engine/platform/windowing/Window.hpp"
#include "engine/platform/input/InputManager.hpp"
#include "engine/platform/filesystem/PathUtils.hpp"
#include "engine/rhi/vulkan/device/Device.hpp"
#include "engine/rhi/vulkan/device/SwapChain.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vulkan_engine::application
{
    ApplicationBase::ApplicationBase(const ApplicationConfig& config)
        : config_(config)
        , running_(false)
    {
    }

    ApplicationBase::~ApplicationBase() = default;

    bool ApplicationBase::initialize()
    {
        // Initialize PathUtils with executable path to find correct project root
        // This ensures assets are found regardless of working directory
        {
            std::filesystem::path exe_path;
            #ifdef _WIN32
            wchar_t buffer[MAX_PATH];
            if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0)
            {
                exe_path = buffer;
            }
            #else
            // Linux: read /proc/self/exe
            if (std::filesystem::exists("/proc/self/exe"))
            {
                exe_path = std::filesystem::read_symlink("/proc/self/exe");
            }
            #endif
            core::PathUtils::initialize(exe_path);
        }

        // Initialize platform
        initialize_platform();

        // Initialize input
        initialize_input();

        // Initialize rendering
        initialize_rendering();

        // Call derived class initialization
        if (!on_initialize())
        {
            std::cerr << "Application-specific initialization failed\n";
            return false;
        }

        running_ = true;
        return true;
    }

    void ApplicationBase::shutdown()
    {
        // Call derived class shutdown
        on_shutdown();

        // Cleanup (reverse order of initialization)
        swap_chain_.reset();
        renderer_.reset();
        input_manager_.reset();
        device_manager_.reset();
        window_.reset();
    }

    void ApplicationBase::run()
    {
        last_frame_time_ = std::chrono::steady_clock::now();

        while (running_)
        {
            // Calculate delta time
            auto  current_time = std::chrono::steady_clock::now();
            float delta_time   = std::chrono::duration<float>(current_time - last_frame_time_).count();
            last_frame_time_   = current_time;

            // Poll window events FIRST (processes GLFW events and triggers callbacks)
            if (window_)
            {
                window_->poll_events();
                if (window_->should_close())
                {
                    request_exit();
                }
            }

            // Update input state SECOND (updates just_pressed/just_released states)
            if (input_manager_)
            {
                input_manager_->update();

                // Check for exit key
                if (input_manager_->is_key_just_pressed(platform::Key::Escape))
                {
                    request_exit();
                }
            }

            // Update application THIRD (uses current input values)
            // Note: scroll_delta is accumulated and reset after reading
            on_update(delta_time);

            // Check if swap chain needs recreation (e.g., window minimized)
            if (swap_chain_ && swap_chain_->needs_recreation())
            {
                auto [width, height] = window_->size();
                if (width > 0 && height > 0)
                {
                    swap_chain_->recreate();
                }
                else
                {
                    // Window is minimized, skip rendering
                    continue;
                }
            }

            // Render LAST
            on_render();
        }
    }

    void ApplicationBase::on_window_resize(const WindowResizeEvent& event)
    {
        config_.width  = event.width;
        config_.height = event.height;

        // Recreate swap chain
        if (swap_chain_)
        {
            swap_chain_->recreate();
        }

        // Notify renderer of resize
        // if (renderer_) { renderer_->on_resize(event.width, event.height); }
    }

    void ApplicationBase::on_key(const KeyEvent& /*event*/)
    {
        // Forward to input manager if available
    }

    void ApplicationBase::on_mouse_button(const MouseButtonEvent& /*event*/)
    {
        // Forward to input manager if available
    }

    void ApplicationBase::on_mouse_move(const MouseMoveEvent& /*event*/)
    {
        // Forward to input manager if available
    }

    void ApplicationBase::on_scroll(const ScrollEvent& /*event*/)
    {
        // Forward to input manager if available
    }

    void ApplicationBase::initialize_platform()
    {
        // Create window
        platform::WindowConfig window_config;
        window_config.title      = config_.title;
        window_config.width      = config_.width;
        window_config.height     = config_.height;
        window_config.vsync      = config_.vsync;
        window_config.fullscreen = config_.fullscreen;
        window_config.resizable  = config_.resizable;

        window_ = std::make_shared<platform::Window>(window_config);

        // Setup window callbacks
        window_->on_close([this]()
        {
            request_exit();
        });

        window_->on_resize([this](uint32_t width, uint32_t height)
        {
            WindowResizeEvent event{width, height};
            on_window_resize(event);
        });
    }

    void ApplicationBase::initialize_rendering()
    {
        // Initialize Vulkan device manager
        vulkan::DeviceManager::CreateInfo device_info;
        device_info.application_name   = config_.title;
        device_info.enable_validation  = config_.enable_validation;
        device_info.enable_debug_utils = config_.enable_validation;

        device_manager_ = std::make_shared<vulkan::DeviceManager>(device_info);
        if (!device_manager_->initialize())
        {
            throw std::runtime_error("Failed to initialize Vulkan device");
        }

        // Create swap chain
        vulkan::SwapChainConfig swap_chain_config;
        swap_chain_config.preferred_present_mode = config_.vsync
                                                       ? VK_PRESENT_MODE_FIFO_KHR
                                                       : VK_PRESENT_MODE_IMMEDIATE_KHR;

        swap_chain_ = std::make_shared<vulkan::SwapChain>(device_manager_, window_, swap_chain_config);
        if (!swap_chain_->initialize())
        {
            throw std::runtime_error("Failed to initialize swap chain");
        }

        // Setup window resize callback for swap chain recreation
        swap_chain_->on_recreate([this](uint32_t width, uint32_t height)
        {
            WindowResizeEvent event{width, height};
            on_window_resize(event);
        });

        // Create renderer (placeholder)
        // renderer_ = std::make_shared<Renderer>(device_manager_, window_, swap_chain_);
    }

    void ApplicationBase::initialize_input()
    {
        input_manager_ = std::make_shared<platform::InputManager>(window_);
        input_manager_->initialize();
    }

    void ApplicationBase::process_events()
    {
        // Process window and input events
    }
} // namespace vulkan_engine::application