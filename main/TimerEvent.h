#pragma once

class Scene;

#define TimerEventCallback(name) void name(Scene&);

namespace TimerCallbackList {
	TimerEventCallback(SpawnEnemy);
	TimerEventCallback(MapScale);
}
