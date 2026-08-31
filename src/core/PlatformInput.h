#pragma once

#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <glm/glm.hpp>

class PlatformInput {
public:
    static inline GLFWwindow* s_Window = nullptr;
    static inline glm::vec2 s_LastMousePos = glm::vec2(0.0f);
    static inline glm::vec2 s_MouseDelta = glm::vec2(0.0f);
    static inline bool s_FirstMouse = true;

    static void Init(GLFWwindow* window) {
        s_Window = window;
    }

    enum Key {
        W = GLFW_KEY_W,
        A = GLFW_KEY_A,
        S = GLFW_KEY_S,
        D = GLFW_KEY_D,
        Space = GLFW_KEY_SPACE,
        F = GLFW_KEY_F,
        E = GLFW_KEY_E,
        Q = GLFW_KEY_Q,
        R = GLFW_KEY_R,
        G = GLFW_KEY_G,
        H = GLFW_KEY_H,
        J = GLFW_KEY_J,
        O = GLFW_KEY_O,
        V = GLFW_KEY_V,
        C = GLFW_KEY_C,
        Tab = GLFW_KEY_TAB,
        I = GLFW_KEY_I,
        Escape = GLFW_KEY_ESCAPE,
        Return = GLFW_KEY_ENTER,
        F3 = GLFW_KEY_F3,
        Up = GLFW_KEY_UP,
        Down = GLFW_KEY_DOWN,
        Left = GLFW_KEY_LEFT,
        Right = GLFW_KEY_RIGHT,
        Num1 = GLFW_KEY_1,
        Num2 = GLFW_KEY_2,
        Num3 = GLFW_KEY_3,
        Num4 = GLFW_KEY_4,
        Numpad1 = GLFW_KEY_KP_1,
        Numpad2 = GLFW_KEY_KP_2,
        Numpad3 = GLFW_KEY_KP_3,
        Numpad4 = GLFW_KEY_KP_4
    };

    enum MouseButton {
        LeftBtn = GLFW_MOUSE_BUTTON_LEFT,
        RightBtn = GLFW_MOUSE_BUTTON_RIGHT
    };

    static bool IsKeyPressed(Key key) {
        if (!s_Window) return false;
        return glfwGetKey(s_Window, (int)key) == GLFW_PRESS;
    }

    static bool IsMouseButtonPressed(MouseButton btn) {
        if (!s_Window) return false;
        return glfwGetMouseButton(s_Window, (int)btn) == GLFW_PRESS;
    }

    static glm::vec2 GetMousePos() {
        if (!s_Window) return glm::vec2(0.0f);
        double x, y;
        glfwGetCursorPos(s_Window, &x, &y);
        return glm::vec2((float)x, (float)y);
    }
};

#else

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Window.hpp>
#include <glm/glm.hpp>

class PlatformInput {
public:
    static inline sf::Window* s_Window = nullptr;

    static void Init(sf::Window* window) {
        s_Window = window;
    }

    enum Key {
        W = sf::Keyboard::W,
        A = sf::Keyboard::A,
        S = sf::Keyboard::S,
        D = sf::Keyboard::D,
        Space = sf::Keyboard::Space,
        F = sf::Keyboard::F,
        E = sf::Keyboard::E,
        Q = sf::Keyboard::Q,
        R = sf::Keyboard::R,
        G = sf::Keyboard::G,
        H = sf::Keyboard::H,
        J = sf::Keyboard::J,
        O = sf::Keyboard::O,
        V = sf::Keyboard::V,
        C = sf::Keyboard::C,
        Tab = sf::Keyboard::Tab,
        I = sf::Keyboard::I,
        Escape = sf::Keyboard::Escape,
        Return = sf::Keyboard::Return,
        F3 = sf::Keyboard::F3,
        Up = sf::Keyboard::Up,
        Down = sf::Keyboard::Down,
        Left = sf::Keyboard::Left,
        Right = sf::Keyboard::Right,
        Num1 = sf::Keyboard::Num1,
        Num2 = sf::Keyboard::Num2,
        Num3 = sf::Keyboard::Num3,
        Num4 = sf::Keyboard::Num4,
        Numpad1 = sf::Keyboard::Numpad1,
        Numpad2 = sf::Keyboard::Numpad2,
        Numpad3 = sf::Keyboard::Numpad3,
        Numpad4 = sf::Keyboard::Numpad4
    };

    enum MouseButton {
        LeftBtn = sf::Mouse::Left,
        RightBtn = sf::Mouse::Right
    };

    static bool IsKeyPressed(Key key) {
        return sf::Keyboard::isKeyPressed((sf::Keyboard::Key)key);
    }

    static bool IsMouseButtonPressed(MouseButton btn) {
        return sf::Mouse::isButtonPressed((sf::Mouse::Button)btn);
    }

    static glm::vec2 GetMousePos() {
        if (!s_Window) return glm::vec2(0.0f);
        sf::Vector2i m = sf::Mouse::getPosition(*s_Window);
        return glm::vec2((float)m.x, (float)m.y);
    }
};

#endif
