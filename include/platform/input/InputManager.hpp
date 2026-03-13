#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <array>

namespace vulkan_engine::platform
{
    class Window;

    enum class Key
    {
        Unknown = 0,
        Space,
        Escape,
        Enter,
        Tab,
        Backspace,
        Delete,
        ArrowRight,
        ArrowLeft,
        ArrowDown,
        ArrowUp,
        PageUp,
        PageDown,
        Home,
        End,
        Insert,
        Shift,
        Ctrl,
        Alt,
        // Letters
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        // Numbers
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        // Function keys
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        KeyCount
    };

    enum class KeyAction
    {
        Press,
        Release,
        Repeat
    };

    enum class MouseButton
    {
        Left = 0,
        Right,
        Middle,
        Button4,
        Button5,
        ButtonCount
    };

    enum class MouseButtonAction
    {
        Press,
        Release
    };

    enum class CursorMode
    {
        Normal,
        Hidden,
        Disabled
    };

    class InputManager
    {
        public:
            using KeyCallback         = std::function<void(Key key, KeyAction action)>;
            using MouseButtonCallback = std::function<void(MouseButton button, MouseButtonAction action)>;
            using MouseMoveCallback   = std::function<void(double x, double y)>;
            using ScrollCallback      = std::function<void(double xoffset, double yoffset)>;

            explicit InputManager(std::shared_ptr<Window> window);
            ~InputManager();

            // Non-copyable
            InputManager(const InputManager&)            = delete;
            InputManager& operator=(const InputManager&) = delete;

            void initialize();
            void update();
            void shutdown();

            // Keyboard input
            bool is_key_pressed(Key key) const;
            bool is_key_just_pressed(Key key) const;
            bool is_key_just_released(Key key) const;

            // Mouse input
            bool is_mouse_button_pressed(MouseButton button) const;
            bool is_mouse_button_just_pressed(MouseButton button) const;
            bool is_mouse_button_just_released(MouseButton button) const;

            std::pair<double, double> mouse_position() const;
            std::pair<double, double> mouse_delta() const;
            double                    scroll_delta() const;

            void set_cursor_mode(CursorMode mode);

            // Callbacks
            void on_key(KeyCallback callback);
            void on_mouse_button(MouseButtonCallback callback);
            void on_mouse_move(MouseMoveCallback callback);
            void on_scroll(ScrollCallback callback);

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;

            static Key map_glfw_key(int glfw_key);
    };
} // namespace vulkan_engine::platform