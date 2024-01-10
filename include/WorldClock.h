#ifndef WORLDCLOCK_H
#define WORLDCLOCK_H

#include <chrono>
#include <iostream>

using local_clock = std::chrono::steady_clock;
using duration = local_clock::duration;
using time_point = local_clock::time_point;
using duration_out = std::chrono::duration<float, std::chrono::seconds::period>;

class WorldClock
{
public:
    static WorldClock* Instance();
    ~WorldClock() {}

    void Update();

    float TotalTime() const { return std::chrono::duration_cast<duration_out>(m_totalTime).count(); }
    float DeltaTime() const { return std::chrono::duration_cast<duration_out>(m_deltaTime).count(); }

private:
    WorldClock() {}

    time_point m_prevClock;
    duration m_totalTime;
    duration m_deltaTime;
};

#endif // WORLDCLOCK_H
