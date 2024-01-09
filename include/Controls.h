#ifndef CONTROLS_H
#define CONTROLS_H

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "Utility.h"

namespace Controls
{
    extern glm::ivec2 MouseOrigin;

    extern glm::ivec2 mousePos;
    extern bool mouseLock;

    extern glm::ivec2 GetMousePos();
    static bool GetMouseLock() { return mouseLock; }

    extern void SetMousePos( glm::ivec2 inMousePos );
    extern void SetMouseLock( bool inMouseLock );

    static bool Left() { return glfwGetKey( Utility::MainWindow, 'A' ); }
    static bool Right() { return glfwGetKey( Utility::MainWindow, 'D' ); }
    static bool Forward() { return glfwGetKey( Utility::MainWindow, 'W' ); }
    static bool Back() { return glfwGetKey( Utility::MainWindow, 'S' ); }
    static bool Up() { return glfwGetKey( Utility::MainWindow, GLFW_KEY_SPACE ); }
    static bool Down() { return glfwGetKey( Utility::MainWindow, GLFW_KEY_LEFT_CONTROL ); }

    static bool Menu() { return glfwGetKey( Utility::MainWindow, GLFW_KEY_ESCAPE ); }

    extern void ResetMousePos();
    static bool LeftClick() { return glfwGetMouseButton( Utility::MainWindow, GLFW_MOUSE_BUTTON_1 ); }

    static void UpdateMousePos( GLFWwindow* window, double x, double y )
    {
        mousePos = glm::ivec2( x, y );
    }
};

#endif // CONTROLS_H
