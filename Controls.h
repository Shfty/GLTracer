#ifndef CONTROLS_H
#define CONTROLS_H

#include <glm/glm.hpp>
#include <GL/glfw.h>


namespace Controls
{
    extern glm::ivec2 MOUSE_ORIGIN;

    extern glm::ivec2 mousePos;
    extern bool mouseLock;

    extern glm::ivec2 GetMousePos();
    static bool GetMouseLock() { return mouseLock; }

    extern void SetMousePos( glm::ivec2 inMousePos );
    static void SetMouseLock( bool inMouseLock ) { mouseLock = inMouseLock; }

    static bool Left() { return glfwGetKey( 'A' ); }
    static bool Right() { return glfwGetKey( 'D' ); }
    static bool Forward() { return glfwGetKey( 'W' ); }
    static bool Back() { return glfwGetKey( 'S' ); }
    static bool Up() { return glfwGetKey( GLFW_KEY_SPACE ); }
    static bool Down() { return glfwGetKey( GLFW_KEY_LCTRL ); }

    static bool Menu() { return glfwGetKey( GLFW_KEY_ESC ); }

    extern void ResetMousePos();
    static bool LeftClick() { return glfwGetMouseButton( GLFW_MOUSE_BUTTON_1 ); }

    static void GLFWCALL UpdateMousePos( int x, int y )
    {
        mousePos = glm::ivec2( x, y );
    }
};

#endif // CONTROLS_H
