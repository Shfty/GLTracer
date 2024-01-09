#include "WorldClock.h"

#include <glm/glm.hpp>
#include <time.h>

const float MIN_TIMESTEP = 0.05f;

WorldClock* WorldClock::Instance()
{
    static WorldClock* instance;

    if( instance == NULL )
    {
        instance = new WorldClock();
    }

    return instance;
}

void WorldClock::Update()
{
    m_prevClock = m_clock;
    m_clock = clock();

    m_deltaTime = static_cast< float >( m_clock - m_prevClock ) / static_cast< float >( CLOCKS_PER_SEC );
    m_deltaTime = glm::min( m_deltaTime, MIN_TIMESTEP );
}
