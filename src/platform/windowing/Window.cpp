#include "platform/windowing/Window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace vulkan_engine::platform
{
    // Internal window data
    struct Window::Impl
    {
        GLFWwindow* handle = nullptr;
        std::string title;
        uint32_t    width        = 0;
        uint32_t    height       = 0;
        bool        should_close = false;

        // Callbacks
        ResizeCallback   resize_callback;
        CloseCallback    close_callback;
        FocusCallback    focus_callback;
        MinimizeCallback minimize_callback;
    };

    Window::Window(const WindowConfig& config)
        : impl_(std::make_unique<Impl>())
    {
        impl_->title  = config.title;
        impl_->width  = config.width;
        impl_->height = config.height;

        // Initialize GLFW if not already done
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Configure GLFW
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

        // Create window
        impl_->handle = glfwCreateWindow(
                                         static_cast<int>(config.width),
                                         static_cast<int>(config.height),
                                         config.title.c_str(),
                                         config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
                                         nullptr
                                        );

        if (!impl_->handle)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        // Set user pointer for callbacks
        glfwSetWindowUserPointer(impl_->handle, this);

        // Setup callbacks
        glfwSetWindowSizeCallback(impl_->handle,
                                  [](GLFWwindow* window, int width, int height)
                                  {
                                      auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
                                      if (self && self->impl_->resize_callback)
                                      {
                                          self->impl_->width  = static_cast<uint32_t>(width);
                                          self->impl_->height = static_cast<uint32_t>(height);
                                          self->impl_->resize_callback(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
                                      }
                                  });

        glfwSetWindowCloseCallback(impl_->handle,
                                   [](GLFWwindow* window)
                                   {
                                       auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
                                       if (self && self->impl_->close_callback)
                                       {
                                           self->impl_->close_callback();
                                       }
                                   });
    }

    Window::~Window()
    {
        if (impl_->handle)
        {
            glfwDestroyWindow(impl_->handle);
        }
        glfwTerminate();
    }

    void Window::poll_events()
    {
        glfwPollEvents();
        impl_->should_close = glfwWindowShouldClose(impl_->handle);
    }

    void Window::wait_events()
    {
        glfwWaitEvents();
    }

    bool Window::should_close() const
    {
        return impl_->should_close;
    }

    void Window::request_close()
    {
        impl_->should_close = true;
        glfwSetWindowShouldClose(impl_->handle, GLFW_TRUE);
    }

    void Window::set_title(const std::string& title)
    {
        impl_->title = title;
        glfwSetWindowTitle(impl_->handle, title.c_str());
    }

    std::string Window::title() const
    {
        return impl_->title;
    }

    void Window::set_size(uint32_t width, uint32_t height)
    {
        impl_->width  = width;
        impl_->height = height;
        glfwSetWindowSize(impl_->handle, static_cast<int>(width), static_cast<int>(height));
    }

    std::pair<uint32_t, uint32_t> Window::size() const
    {
        return {impl_->width, impl_->height};
    }

    uint32_t Window::width() const
    {
        return impl_->width;
    }

    uint32_t Window::height() const
    {
        return impl_->height;
    }

    void Window::set_position(int32_t x, int32_t y)
    {
        glfwSetWindowPos(impl_->handle, static_cast<int>(x), static_cast<int>(y));
    }

    std::pair<int32_t, int32_t> Window::position() const
    {
        int x, y;
        glfwGetWindowPos(impl_->handle, &x, &y);
        return {static_cast<int32_t>(x), static_cast<int32_t>(y)};
    }

    void Window::set_fullscreen(bool fullscreen)
    {
        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        glfwSetWindowMonitor(impl_->handle, monitor, 0, 0, impl_->width, impl_->height, GLFW_DONT_CARE);
    }

    bool Window::is_fullscreen() const
    {
        return glfwGetWindowMonitor(impl_->handle) != nullptr;
    }

    void Window::set_vsync(bool enabled)
    {
        // VSync is handled by the swap chain, not GLFW directly
        // This is a placeholder for when swap chain is implemented
    }

    void Window::minimize()
    {
        glfwIconifyWindow(impl_->handle);
    }

    void Window::maximize()
    {
        glfwMaximizeWindow(impl_->handle);
    }

    void Window::restore()
    {
        glfwRestoreWindow(impl_->handle);
    }

    void Window::focus()
    {
        glfwFocusWindow(impl_->handle);
    }

    bool Window::is_minimized() const
    {
        return glfwGetWindowAttrib(impl_->handle, GLFW_ICONIFIED);
    }

    bool Window::is_focused() const
    {
        return glfwGetWindowAttrib(impl_->handle, GLFW_FOCUSED);
    }

    void* Window::native_handle() const
    {
        return impl_->handle;
    }

    VkSurfaceKHR Window::create_surface(VkInstance instance) const
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(instance, impl_->handle, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface");
        }
        return surface;
    }

    std::vector<const char*> Window::get_required_extensions() const
    {
        uint32_t     count      = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char*>(extensions, extensions + count);
    }

    void Window::on_resize(ResizeCallback callback)
    {
        impl_->resize_callback = std::move(callback);
    }

    void Window::on_close(CloseCallback callback)
    {
        impl_->close_callback = std::move(callback);
    }

    void Window::on_focus(FocusCallback callback)
    {
        impl_->focus_callback = std::move(callback);
    }

    void Window::on_minimize(MinimizeCallback callback)
    {
        impl_->minimize_callback = std::move(callback);
    }
} // namespace vulkan_engine::platform