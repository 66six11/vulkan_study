#pragma once

#include <memory>
#include <string>
#include <concepts>
#include <chrono>

namespace vulkan_engine::platform
{
    class Window;
    class InputManager;
}

namespace vulkan_engine::application
{
    // Forward declarations
    class Renderer;

    // Application configuration
    struct ApplicationConfig
    {
        std::string title             = "Vulkan Engine";
        uint32_t    width             = 1280;
        uint32_t    height            = 720;
        bool        vsync             = true;
        bool        fullscreen        = false;
        bool        resizable         = true;
        bool        enable_validation = true;
        bool        enable_profiling  = true;

        // Renderer settings
        bool use_render_graph  = true;
        bool use_async_loading = true;
        bool use_hot_reload    = true;

        // Platform-specific settings
        struct
        {
            bool enable_console = true;
        } windows;

        struct
        {
            bool enable_wayland = true;
        } linux;
    };

    // Application events
    struct WindowResizeEvent
    {
        uint32_t width;
        uint32_t height;
    };

    struct KeyEvent
    {
        int key;
        int scancode;
        int action;
        int mods;
    };

    struct MouseButtonEvent
    {
        int button;
        int action;
        int mods;
    };

    struct MouseMoveEvent
    {
        double x;
        double y;
    };

    struct ScrollEvent
    {
        double xoffset;
        double yoffset;
    };

    // Application concept
    template <typename T>
    concept Application = requires(T app)
    {
        { app.initialize() } -> std::same_as<bool>;
        { app.shutdown() } -> std::same_as<void>;
        { app.update(std::chrono::duration<float>{}) } -> std::same_as<void>;
        { app.render() } -> std::same_as<void>;
    };

    // Main application class
    class ApplicationBase
    {
        public:
            explicit ApplicationBase(const ApplicationConfig& config);
            virtual  ~ApplicationBase();

            // Core lifecycle
            bool initialize();
            void shutdown();
            void run();

            // Virtual methods for derived applications
            virtual bool on_initialize() = 0;
            virtual void on_shutdown() = 0;
            virtual void on_update(float delta_time) = 0;
            virtual void on_render() = 0;

            // Event handlers
            virtual void on_window_resize(const WindowResizeEvent& event);
            virtual void on_key(const KeyEvent& event);
            virtual void on_mouse_button(const MouseButtonEvent& event);
            virtual void on_mouse_move(const MouseMoveEvent& event);
            virtual void on_scroll(const ScrollEvent& event);

            // Accessors
            std::shared_ptr<platform::Window>       window() const { return window_; }
            std::shared_ptr<Renderer>               renderer() const { return renderer_; }
            std::shared_ptr<platform::InputManager> input_manager() const { return input_manager_; }

            const ApplicationConfig& config() const { return config_; }
            bool                     running() const { return running_; }

        protected:
            void request_exit() { running_ = false; }

        private:
            ApplicationConfig                       config_;
            std::shared_ptr<platform::Window>       window_;
            std::shared_ptr<Renderer>               renderer_;
            std::shared_ptr<platform::InputManager> input_manager_;
            bool                                    running_ = false;

            // Timing
            std::chrono::steady_clock::time_point last_frame_time_;

            void initialize_platform();
            void initialize_rendering();
            void initialize_input();
            void process_events();
    };

    // Application factory function
    template <Application App, typename... Args> std::unique_ptr<ApplicationBase> create_application(Args&&... args)
    {
        return std::make_unique<App>(std::forward<Args>(args)...);
    }

    // Example application implementation
    template <typename UpdateFunc, typename RenderFunc> class LambdaApplication : public ApplicationBase
    {
        public:
            LambdaApplication(
                const ApplicationConfig& config,
                UpdateFunc&&             update_func,
                RenderFunc&&             render_func)
                : ApplicationBase(config)
                , update_func_{std::forward<UpdateFunc>(update_func)}
                , render_func_{std::forward<RenderFunc>(render_func)}
            {
            }

            bool on_initialize() override
            {
                // Custom initialization logic
                return true;
            }

            void on_shutdown() override
            {
                // Custom shutdown logic
            }

            void on_update(float delta_time) override
            {
                update_func_(delta_time);
            }

            void on_render() override
            {
                render_func_();
            }

        private:
            UpdateFunc update_func_;
            RenderFunc render_func_;
    };
} // namespace vulkan_engine::application