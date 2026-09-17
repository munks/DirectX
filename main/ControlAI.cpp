#include "ControlAI.h"
#include "GameState.h"

// ControlAI.cpp
// bool ControlAI (Object::Rectangle& object, float deltaSeconds);

namespace ControlAI {
    bool MoveRight(Object::Rectangle& object, float deltaSeconds, GameState&, float scale) {
        object.x += object.speed * scale * deltaSeconds;
        return false;
    }
	bool MoveToPlayer(Object::Rectangle& object, float deltaSeconds, GameState& player, float scale) {
		float xDirection = 0.0f;
		float yDirection = 0.0f;
		if (!player._isAlive) return false;
		if (player.player.x < object.x) xDirection -= 1.0f;
		if (player.player.x > object.x) xDirection += 1.0f;
		if (player.player.y < object.y) yDirection -= 1.0f;
		if (player.player.y > object.y) yDirection += 1.0f;
		const float scaledSpeed = object.speed * scale;
		object.x += xDirection * scaledSpeed * deltaSeconds;
		object.y += yDirection * scaledSpeed * deltaSeconds;
		// Return true if the object has reached the player
		const float distanceSquared = (object.x - player.player.x) * (object.x - player.player.x) +
			(object.y - player.player.y) * (object.y - player.player.y);
		return distanceSquared < 1.0f; // Considered "reached" if within 1 pixel
	}
	bool Stop(Object::Rectangle&, float, GameState&, float) {
		return false;
	}
}

namespace ImpactFunc {
	Object::RemoveTarget DestroyObject(GameScene&, Object::Rectangle&, GameState&) {
		return Object::RemoveTarget::Object;
	}

	Object::RemoveTarget DamageToPlayer(GameScene&, Object::Rectangle&, GameState& gameState) {
		ApplyDamage(gameState);
		return Object::RemoveTarget::Object;
	}

	Object::RemoveTarget DestroyRandomEnemy(GameScene&, Object::Rectangle&, GameState&) {
		return Object::RemoveTarget::RandomEnemy;
	}
}
