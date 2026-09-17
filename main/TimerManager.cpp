#include "TimerManager.h"

#include "Scene/GameScene.h"

#include <algorithm>

TimerId TimerManager::Add(float delaySeconds, float intervalSeconds, bool repeat, TimerCallback callback) {
    const TimerEvent event{
        _nextId++,
        std::max(0.0f, delaySeconds),
        intervalSeconds,
        repeat,
        false,
        callback
    };

    if (_isUpdating) {
        _pendingEvents.push_back(event);
    } else {
        _events.push_back(event);
    }
    return event.id;
}

TimerId TimerManager::AddOnce(float delaySeconds, TimerCallback callback) {
    return Add(delaySeconds, 0.0f, false, callback);
}

TimerId TimerManager::AddRepeat(float intervalSeconds, TimerCallback callback) {
    if (intervalSeconds <= 0.0f) {
        return 0;
    }
    return Add(intervalSeconds, intervalSeconds, true, callback);
}

TimerId TimerManager::AddRepeatAfter(float delaySeconds, float intervalSeconds, TimerCallback callback) {
    if (intervalSeconds <= 0.0f) {
        return 0;
    }
    return Add(delaySeconds, intervalSeconds, true, callback);
}

void TimerManager::Cancel(TimerId id) {
    for (TimerEvent& event : _events) {
        if (event.id == id) {
            event.canceled = true;
        }
    }
    for (TimerEvent& event : _pendingEvents) {
        if (event.id == id) {
            event.canceled = true;
        }
    }
}

void TimerManager::Update(GameScene& scene, float deltaSeconds) {
    // 콜백 안에서 등록된 이벤트는 _pendingEvents에 넣어 이번 순회에서는 실행하지 않는다.
    _isUpdating = true;

    for (auto iterator = _events.begin(); iterator != _events.end();) {
        if (iterator->canceled) {
            iterator = _events.erase(iterator);
            continue;
        }

        iterator->remainingSeconds -= deltaSeconds;
        if (iterator->remainingSeconds > 0.0f) {
            ++iterator;
            continue;
        }

        if (iterator->callback) {
            iterator->callback(scene);
        }

        if (iterator->canceled || !iterator->repeat) {
            iterator = _events.erase(iterator);
            continue;
        }

        do {
            iterator->remainingSeconds += iterator->intervalSeconds;
        } while (iterator->remainingSeconds <= 0.0f);
        ++iterator;
    }

    _isUpdating = false;
    for (const TimerEvent& event : _pendingEvents) {
        if (!event.canceled) {
            _events.push_back(event);
        }
    }
    _pendingEvents.clear();
}
