#include "Controls.h"

namespace Controls
{
    glm::ivec2 mousePos = glm::ivec2( 0, 0 );
    bool mouseLock = true;

    void SetMouseLock( bool inMouseLock )
    {
        mouseLock = inMouseLock;
        if( mouseLock )
        {
            glfwSetInputMode( Utility::MainWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN );
        }
        else
        {
            glfwSetInputMode( Utility::MainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
        }
    }

    void ResetMousePos()
    {
        SetMousePos( MouseOrigin );
    }

    glm::ivec2 GetMousePos()
    {
        if( mouseLock )
        {
            return mousePos;
        }
        else
        {
            return MouseOrigin;
        }
    }

    void SetMousePos( glm::ivec2 inMousePos )
    {
        glfwSetCursorPos( Utility::MainWindow, inMousePos.x, inMousePos.y );
        mousePos = inMousePos;
    }
}
