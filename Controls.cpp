#include "Controls.h"

namespace Controls
{
    glm::ivec2 MOUSE_ORIGIN = glm::ivec2( 0, 0 );

    glm::ivec2 mousePos = glm::ivec2( 0, 0 );
    bool mouseLock = true;

    void ResetMousePos()
    {
        SetMousePos( MOUSE_ORIGIN );
    }

    glm::ivec2 GetMousePos()
    {
        if( mouseLock )
        {
            return mousePos;
        }
        else
        {
            return glm::ivec2( 0, 0 );
        }
    }

    void SetMousePos( glm::ivec2 inMousePos )
    {
        if( mouseLock )
        {
            glfwSetMousePos( inMousePos.x, inMousePos.y );
            mousePos = inMousePos;
        }
    }
}
