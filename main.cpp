#include <ctime>

#include "WorldClock.h"
#include "GLTracer.h"

int main()
{
    GLTracer glTracer;

    while( true )
    {
        // Cap update to tick rate
        WorldClock::Instance()->Update();

        // Update and Draw renderer
        glTracer.Update();
        glTracer.Draw();
    }
    return 0;
}
