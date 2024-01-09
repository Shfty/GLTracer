#ifndef WORLDCLOCK_H
#define WORLDCLOCK_H

#include <time.h>
#include <iostream>

class WorldClock
{
public:
    static WorldClock* Instance();
    ~WorldClock() {}

    void Update();

    int Clock() const { return m_clock; }
    int ElapsedTime() const { return m_clock - m_prevClock; }
    float DeltaTime() const { return m_deltaTime; }

private:
    WorldClock() {}

    int m_clock = 0;
    int m_prevClock = 0;
    float m_deltaTime = 0.0f;
};

#endif // WORLDCLOCK_H
