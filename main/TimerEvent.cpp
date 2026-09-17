
#include "TimerEvent.h"

#include "ControlAI.h"
#include "Scene/GameScene.h"
#include "main.h"

constexpr int MAX_ATTEMPT = 100;

static void GetRandomPointInDevice(GameScene& pScene, SceneData& data, float offset) {
	const float virtualLeft = (float)GetSystemMetrics(SM_XVIRTUALSCREEN);
	const float virtualTop = (float)GetSystemMetrics(SM_YVIRTUALSCREEN);
	const float virtualRight = virtualLeft + (float)GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const float virtualBottom = virtualTop + (float)GetSystemMetrics(SM_CYVIRTUALSCREEN);

	std::uniform_real_distribution distX(virtualLeft + offset, virtualRight - offset);
	std::uniform_real_distribution distY(virtualTop + offset, virtualBottom - offset);
	for (int i = 0; i < MAX_ATTEMPT; i++) {
		data.x = distX(::random_engine);
		data.y = distY(::random_engine);
		if (data.x >= pScene.GetGameState().player.x - 300.f && data.x <= pScene.GetGameState().player.x) {
			continue;
		}
		if (data.y >= pScene.GetGameState().player.y - 300.f && data.y <= pScene.GetGameState().player.y) {
			continue;
		}
		break;
	}
}

namespace TimerCallbackList {
	void SpawnEnemy(GameScene& pScene) {
		SceneData data;

		//Create Player Chase Enemy
		GetRandomPointInDevice(pScene, data, 0.0f);
		data.width = 60.0f;
		data.height = 60.0f;
		data.r = 0.20f;
		data.g = 0.20f;
		data.b = 0.80f;
		data.speed = 160.0f;
		pScene.CreateObject(data, Object::Type::Enemy, ControlAI::MoveToPlayer, ImpactFunc::DamageToPlayer);

		//Create Remove Random Enemy Object
		GetRandomPointInDevice(pScene, data, 100.0f);
		data.width = 30.0f;
		data.height = 30.0f;
		data.r = 0.00f;
		data.g = 1.00f;
		data.b = 0.00f;
		pScene.CreateObject(data, Object::Type::Neutral, ControlAI::Stop, ImpactFunc::DestroyRandomEnemy);
	}
	
	void MapScale(GameScene& pScene) {
		pScene.SetScaleTrans(pScene.GetScale() - 0.1f, 3.0f);
	}
}
