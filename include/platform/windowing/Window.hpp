#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>
#include <vector>

// Forward declaration for Vulkan handle
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
typedef struct VkInstance_T*   VkInstance;

namespace vulkan_engine::platform
{
    struct WindowConfig
    {
        std::string title      = "Vulkan Engine";
        uint32_t    width      = 1280;
        uint32_t    height     = 720;
        bool        vsync      = true;
        bool        fullscreen = false;
        bool        resizable  = true;
    };

    class Window
    {
        public:
            using ResizeCallback   = std::function<void(uint32_t width, uint32_t height)>;
            using CloseCallback    = std::function<void()>;
            using FocusCallback    = std::function<void(bool focused)>;
            using MinimizeCallback = std::function<void(bool minimized)>;

            explicit Window(const WindowConfig& config);
            ~Window();

            // Non-copyable
            Window(const Window&)            = delete;
            Window& operator=(const Window&) = delete;

            // Movable
            Window(Window&&) noexcept            = default;
            Window& operator=(Window&&) noexcept = default;

            // Event processing
            void poll_events();
            void wait_events();

            // Window state
            bool should_close() const;
            void request_close();

            // Window properties
            void                          set_title(const std::string& title);
            std::string                   title() const;
            void                          set_size(uint32_t width, uint32_t height);
            std::pair<uint32_t, uint32_t> size() const;
            uint32_t                      width() const;
            uint32_t                      height() const;
            void                          set_position(int32_t x, int32_t y);
            std::pair<int32_t, int32_t>   position() const;

            // Window modes
            void set_fullscreen(bool fullscreen);
            bool is_fullscreen() const;
            void set_vsync(bool enabled);

            // Window state changes
            void minimize();
            void maximize();
            void restore();
            void focus();

            bool is_minimized() const;
            bool is_focused() const;

            // Native handle access
            void* native_handle() const;

            // Vulkan integration
            VkSurfaceKHR             create_surface(VkInstance instance) const;
            std::vector<const char*> get_required_extensions() const;

            // Callbacks
            void on_resize(ResizeCallback callback);
            void on_close(CloseCallback callback);
            void on_focus(FocusCallback callback);
            void on_minimize(MinimizeCallback callback);

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace vulkan_engine::platform