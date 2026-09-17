#pragma once

class GameScene;

#define TimerEventCallback(name) void name(GameScene&);

namespace TimerCallbackList {
	TimerEventCallback(SpawnEnemy);
	TimerEventCallback(MapScale);
}
