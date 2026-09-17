#pragma once

#include "Renderer.h"

struct GameState;

// bool ControlAI (Object::Rectangle& object, float deltaSeconds, Object::Rectangle& player);
#define ControlAIDefine(name) bool name(Object::Rectangle& object, float deltaSeconds, GameState& player, float scale)
#define ImpactFuncDefine(name) Object::RemoveTarget name(GameScene& scene, Object::Rectangle& object, GameState& player)

namespace ControlAI {
    ControlAIDefine(MoveRight);
    ControlAIDefine(MoveToPlayer);
	ControlAIDefine(Stop);
}

namespace ImpactFunc {
	ImpactFuncDefine(DestroyObject);
	ImpactFuncDefine(DamageToPlayer);
	ImpactFuncDefine(DestroyRandomEnemy);
}
