#include "platform/input/InputManager.hpp"
#include "platform/windowing/Window.hpp"
#include <GLFW/glfw3.h>
#include <array>

namespace vulkan_engine::platform
{
    // Internal input state
    struct InputManager::Impl
    {
        std::shared_ptr<Window> window;

        // Keyboard state
        std::array<bool, static_cast<size_t>(Key::KeyCount)> keys_current{};
        std::array<bool, static_cast<size_t>(Key::KeyCount)> keys_previous{};

        // Mouse state
        std::array<bool, static_cast<size_t>(MouseButton::ButtonCount)> mouse_current{};
        std::array<bool, static_cast<size_t>(MouseButton::ButtonCount)> mouse_previous{};

        double mouse_x       = 0.0;
        double mouse_y       = 0.0;
        double mouse_delta_x = 0.0;
        double mouse_delta_y = 0.0;
        double scroll_delta  = 0.0;

        // Callbacks
        KeyCallback         key_callback;
        MouseButtonCallback mouse_button_callback;
        MouseMoveCallback   mouse_move_callback;
        ScrollCallback      scroll_callback;
    };

    InputManager::InputManager(std::shared_ptr<Window> window)
        : impl_(std::make_unique<Impl>())
    {
        impl_->window = std::move(window);
    }

    InputManager::~InputManager() = default;

    void InputManager::initialize()
    {
        GLFWwindow* glfw_window = static_cast<GLFWwindow*>(impl_->window->native_handle());

        // Set up key callback
        glfwSetKeyCallback(glfw_window,
                           [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
                           {
                               auto* self = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
                               if (self && self->impl_->key_callback)
                               {
                                   Key       mapped_key    = map_glfw_key(key);
                                   KeyAction mapped_action = (action == GLFW_PRESS)
                                                                 ? KeyAction::Press
                                                                 : (action == GLFW_RELEASE)
                                                                 ? KeyAction::Release
                                                                 : KeyAction::Repeat;
                                   self->impl_->key_callback(mapped_key, mapped_action);
                               }
                           });

        // Set up mouse button callback
        glfwSetMouseButtonCallback(glfw_window,
                                   [](GLFWwindow* window, int button, int action, int /*mods*/)
                                   {
                                       auto* self = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
                                       if (self && self->impl_->mouse_button_callback)
                                       {
                                           MouseButton mapped_button = (button == GLFW_MOUSE_BUTTON_LEFT)
                                                                           ? MouseButton::Left
                                                                           : (button == GLFW_MOUSE_BUTTON_RIGHT)
                                                                           ? MouseButton::Right
                                                                           : (button == GLFW_MOUSE_BUTTON_MIDDLE)
                                                                           ? MouseButton::Middle
                                                                           : MouseButton::Button4;
                                           MouseButtonAction mapped_action = (action == GLFW_PRESS)
                                                                                 ? MouseButtonAction::Press
                                                                                 : MouseButtonAction::Release;
                                           self->impl_->mouse_button_callback(mapped_button, mapped_action);
                                       }
                                   });

        // Set up cursor position callback
        glfwSetCursorPosCallback(glfw_window,
                                 [](GLFWwindow* window, double xpos, double ypos)
                                 {
                                     auto* self = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
                                     if (self)
                                     {
                                         self->impl_->mouse_delta_x = xpos - self->impl_->mouse_x;
                                         self->impl_->mouse_delta_y = ypos - self->impl_->mouse_y;
                                         self->impl_->mouse_x       = xpos;
                                         self->impl_->mouse_y       = ypos;

                                         if (self->impl_->mouse_move_callback)
                                         {
                                             self->impl_->mouse_move_callback(xpos, ypos);
                                         }
                                     }
                                 });

        // Set up scroll callback
        glfwSetScrollCallback(glfw_window,
                              [](GLFWwindow* window, double xoffset, double yoffset)
                              {
                                  auto* self = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
                                  if (self)
                                  {
                                      self->impl_->scroll_delta = yoffset;
                                      if (self->impl_->scroll_callback)
                                      {
                                          self->impl_->scroll_callback(xoffset, yoffset);
                                      }
                                  }
                              });

        glfwSetWindowUserPointer(glfw_window, this);
    }

    void InputManager::update()
    {
        // Update previous state
        impl_->keys_previous  = impl_->keys_current;
        impl_->mouse_previous = impl_->mouse_current;
        impl_->mouse_delta_x  = 0.0;
        impl_->mouse_delta_y  = 0.0;
        impl_->scroll_delta   = 0.0;

        GLFWwindow* glfw_window = static_cast<GLFWwindow*>(impl_->window->native_handle());

        // Update keyboard state - only query mapped keys
        static constexpr std::array<int, static_cast<size_t>(Key::KeyCount)> key_map = {
            GLFW_KEY_UNKNOWN,    // Unknown
            GLFW_KEY_SPACE,      // Space
            GLFW_KEY_ESCAPE,     // Escape
            GLFW_KEY_ENTER,      // Enter
            GLFW_KEY_TAB,        // Tab
            GLFW_KEY_BACKSPACE,  // Backspace
            GLFW_KEY_DELETE,     // Delete
            GLFW_KEY_RIGHT,      // ArrowRight
            GLFW_KEY_LEFT,       // ArrowLeft
            GLFW_KEY_DOWN,       // ArrowDown
            GLFW_KEY_UP,         // ArrowUp
            GLFW_KEY_PAGE_UP,    // PageUp
            GLFW_KEY_PAGE_DOWN,  // PageDown
            GLFW_KEY_HOME,       // Home
            GLFW_KEY_END,        // End
            GLFW_KEY_INSERT,     // Insert
            GLFW_KEY_LEFT_SHIFT, // Shift
            GLFW_KEY_LEFT_CONTROL, // Ctrl
            GLFW_KEY_LEFT_ALT,   // Alt
            // A-Z
            GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F,
            GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L,
            GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R,
            GLFW_KEY_S, GLFW_KEY_T, GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X,
            GLFW_KEY_Y, GLFW_KEY_Z,
            // Numbers
            GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
            GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
            // Function keys
            GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4,
            GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8,
            GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12,
        };

        for (size_t i = 0; i < static_cast<size_t>(Key::KeyCount); ++i)
        {
            if (key_map[i] != GLFW_KEY_UNKNOWN)
            {
                impl_->keys_current[i] = glfwGetKey(glfw_window, key_map[i]) == GLFW_PRESS;
            }
        }

        // Update mouse state - only query mapped buttons
        static constexpr std::array<int, static_cast<size_t>(MouseButton::ButtonCount)> button_map = {
            GLFW_MOUSE_BUTTON_LEFT,    // Left
            GLFW_MOUSE_BUTTON_RIGHT,   // Right
            GLFW_MOUSE_BUTTON_MIDDLE,  // Middle
            GLFW_MOUSE_BUTTON_4,       // Button4
            GLFW_MOUSE_BUTTON_5,       // Button5
        };

        for (size_t i = 0; i < static_cast<size_t>(MouseButton::ButtonCount); ++i)
        {
            impl_->mouse_current[i] = glfwGetMouseButton(glfw_window, button_map[i]) == GLFW_PRESS;
        }
    }

    void InputManager::shutdown()
    {
        // Reset callbacks
        GLFWwindow* glfw_window = static_cast<GLFWwindow*>(impl_->window->native_handle());
        glfwSetKeyCallback(glfw_window, nullptr);
        glfwSetMouseButtonCallback(glfw_window, nullptr);
        glfwSetCursorPosCallback(glfw_window, nullptr);
        glfwSetScrollCallback(glfw_window, nullptr);
    }

    bool InputManager::is_key_pressed(Key key) const
    {
        return impl_->keys_current[static_cast<size_t>(key)];
    }

    bool InputManager::is_key_just_pressed(Key key) const
    {
        auto idx = static_cast<size_t>(key);
        return impl_->keys_current[idx] && !impl_->keys_previous[idx];
    }

    bool InputManager::is_key_just_released(Key key) const
    {
        auto idx = static_cast<size_t>(key);
        return !impl_->keys_current[idx] && impl_->keys_previous[idx];
    }

    bool InputManager::is_mouse_button_pressed(MouseButton button) const
    {
        return impl_->mouse_current[static_cast<size_t>(button)];
    }

    bool InputManager::is_mouse_button_just_pressed(MouseButton button) const
    {
        auto idx = static_cast<size_t>(button);
        return impl_->mouse_current[idx] && !impl_->mouse_previous[idx];
    }

    bool InputManager::is_mouse_button_just_released(MouseButton button) const
    {
        auto idx = static_cast<size_t>(button);
        return !impl_->mouse_current[idx] && impl_->mouse_previous[idx];
    }

    std::pair<double, double> InputManager::mouse_position() const
    {
        return {impl_->mouse_x, impl_->mouse_y};
    }

    std::pair<double, double> InputManager::mouse_delta() const
    {
        return {impl_->mouse_delta_x, impl_->mouse_delta_y};
    }

    double InputManager::scroll_delta() const
    {
        return impl_->scroll_delta;
    }

    void InputManager::set_cursor_mode(CursorMode mode)
    {
        GLFWwindow* glfw_window = static_cast<GLFWwindow*>(impl_->window->native_handle());
        int         glfw_mode   = (mode == CursorMode::Normal)
                                      ? GLFW_CURSOR_NORMAL
                                      : (mode == CursorMode::Hidden)
                                      ? GLFW_CURSOR_HIDDEN
                                      : GLFW_CURSOR_DISABLED;
        glfwSetInputMode(glfw_window, GLFW_CURSOR, glfw_mode);
    }

    void InputManager::on_key(KeyCallback callback)
    {
        impl_->key_callback = std::move(callback);
    }

    void InputManager::on_mouse_button(MouseButtonCallback callback)
    {
        impl_->mouse_button_callback = std::move(callback);
    }

    void InputManager::on_mouse_move(MouseMoveCallback callback)
    {
        impl_->mouse_move_callback = std::move(callback);
    }

    void InputManager::on_scroll(ScrollCallback callback)
    {
        impl_->scroll_callback = std::move(callback);
    }

    Key InputManager::map_glfw_key(int glfw_key)
    {
        // Simplified mapping - in a real implementation, map all GLFW keys
        switch (glfw_key)
        {
            case GLFW_KEY_SPACE: return Key::Space;
            case GLFW_KEY_ESCAPE: return Key::Escape;
            case GLFW_KEY_W: return Key::W;
            case GLFW_KEY_A: return Key::A;
            case GLFW_KEY_S: return Key::S;
            case GLFW_KEY_D: return Key::D;
            default: return Key::Unknown;
        }
    }
} // namespace vulkan_engine::platform