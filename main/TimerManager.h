#pragma once

#include <cstdint>
#include <vector>

class GameScene;

using TimerId = std::uint32_t;
using TimerCallback = void (*)(GameScene& scene);

struct TimerEvent {
    TimerId id = 0;
    float remainingSeconds = 0.0f;
    float intervalSeconds = 0.0f;
    bool repeat = false;
    bool canceled = false;
    TimerCallback callback = nullptr;
};

class TimerManager {
public:
    TimerId AddOnce(float delaySeconds, TimerCallback callback);
    TimerId AddRepeat(float intervalSeconds, TimerCallback callback);
    TimerId AddRepeatAfter(float delaySeconds, float intervalSeconds, TimerCallback callback);
    void Cancel(TimerId id);
    void Update(GameScene& scene, float deltaSeconds);

private:
    TimerId Add(float delaySeconds, float intervalSeconds, bool repeat, TimerCallback callback);

    std::vector<TimerEvent> _events;
    std::vector<TimerEvent> _pendingEvents;
    TimerId _nextId = 1;
    bool _isUpdating = false;
};
