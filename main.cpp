#include <ctime>

#include "WorldClock.h"
#include "GLTracer.h"

int main()
{
    GLTracer glTracer;

    while( true )
    {
        // Update and Draw renderer
        glTracer.Update();
        glTracer.Draw();
    }
    return 0;
}
