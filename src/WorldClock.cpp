#include "WorldClock.h"

#include <glm/glm.hpp>
#include <time.h>

WorldClock* WorldClock::Instance()
{
    static WorldClock* instance;

    if( instance == NULL )
    {
        instance = new WorldClock();
        instance->m_prevClock = local_clock::now();
    }

    return instance;
}

void WorldClock::Update()
{
    const time_point clock = local_clock::now();
    m_deltaTime = clock - m_prevClock;
    m_totalTime += m_deltaTime;
    m_prevClock = clock;
}
