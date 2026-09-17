#include "GameState.h"

#include "main.h"
#include "UI.h"

#include <algorithm>
#include <cwchar>
#include <iterator>

void UpdateLifeText(const GameState& gameState) {
    wchar_t hearts[5] = L"";
    const int visibleLife = std::min(gameState._life.Get(), static_cast<int>(std::size(hearts) - 1));

    for (int index = 0; index < visibleLife; ++index) {
        // U+2661: 빈 하트를 표시한다.
        wcscat_s(hearts, std::size(hearts), L"\u2661");
    }
    UI::SetText(TEXT_UI_LIFE, hearts);
}

void ApplyDamage(GameState& gameState, int damage) {
    if (damage <= 0 || !gameState._isAlive) {
        return;
    }

    gameState._life.Set(std::max(0, gameState._life.Get() - damage));
    if (gameState._life.Get() == 0) {
        gameState._isAlive = false;
        UI::SetText(TEXT_UI_GAMEOVER, L"Game Over\nPress F5 to Restart");
    }
    UpdateLifeText(gameState);
}
