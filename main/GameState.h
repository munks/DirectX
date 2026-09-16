#pragma once

#include "ProtectedValue.h"
#include "Rectangle.h"

struct GameState {
    Object::Rectangle player;
    bool _isAlive = true;
    Memory::ProtectedInt _life = Memory::ProtectedInt(3);
};

void ApplyDamage(GameState& gameState, int damage = 1);
void UpdateLifeText(const GameState& gameState);
