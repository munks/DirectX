#pragma once

#include <memory>
#include <random>

#include "GameState.h"
#include "Scene.h"

extern std::mt19937 random_engine;
extern GameState gGameState;
extern std::unique_ptr<Scene> gScene;

constexpr UI::TextId TEXT_UI_LIFE = 1;
constexpr UI::TextId TEXT_UI_GAMEOVER = 2;
constexpr UI::TextFormatId TEXT_FORMAT_UI = 1;
constexpr UI::TextFormatId TEXT_FORMAT_GAMEOVER = 2;
