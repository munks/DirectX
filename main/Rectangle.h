#pragma once

#include <windows.h>
#include <algorithm>
#include <cstdint>

class GameScene;
struct GameState;

namespace Object {
    using ObjectId = std::uint64_t;

    struct Rectangle;
    enum class RemoveTarget {
        None,
        Object,
        Player,
        RandomEnemy
    };

	enum class Type {
		Enemy,
		Neutral,
        Undefined
	};

    using ControlAI = bool (*)(Rectangle& object, float deltaSeconds, GameState& player, float scale);
    using ImpactFunc = RemoveTarget (*)(GameScene& scene, Rectangle& object, GameState& player);

    struct Rectangle {
        ObjectId id = 0;

        // Center position in desktop screen pixels.
        float x = 0.0f;
        float y = 0.0f;
        float width = 80.0f;
        float height = 80.0f;
        float speed = 0.0f;
        float red = 1.0f;
        float green = 1.0f;
        float blue = 1.0f;
		Type type = Type::Undefined;
		ControlAI controlAI = nullptr;
		ImpactFunc impact = nullptr;
    };

    struct RectangleConstants {
        float position[2];
        float viewportSize[2];
        float size[2];
        float sizePadding[2];
        float color[3];
        float colorPadding;
    };
}
