#include "MainScene.h"

#include "GameScene.h"
#include "../main.h"
#include "../UI.h"

#include <cmath>
#include <string>

namespace {
    constexpr UI::TextId kStartTextId = 100;
    constexpr float kCountdownSeconds = 3.0f;
}

void MainScene::Initialize(_In_ Renderer* renderer) {
    SceneBase::Initialize(renderer);
    _sceneTexts.clear();
    _sceneTexts.emplace(kStartTextId, UI::TextDrawRequest{
        L"Press Space to Start", TEXT_FORMAT_GAMEOVER, UI::TextAnchor::Center,
        0.0f, 0.0f, 500.0f, 80.0f
    });
    _wasSpaceDown = false;
    _isCountingDown = false;
}

void MainScene::Update() {
    const bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    if (!_isCountingDown && isSpaceDown && !_wasSpaceDown) {
        _isCountingDown = true;
        _countdownStart = std::chrono::steady_clock::now();
    }
    _wasSpaceDown = isSpaceDown;

    if (_isCountingDown) {
        const float elapsedSeconds = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - _countdownStart).count();

        if (elapsedSeconds >= kCountdownSeconds) {
            // 전환은 SceneBase가 Update 후 처리하므로 이 MainScene은 안전하게 파기된다.
            SceneBase::RequestSceneChange(std::make_unique<GameScene>(_gameData));
        }
        else {
            const int remainingSeconds = static_cast<int>(std::ceil(kCountdownSeconds - elapsedSeconds));
            _sceneTexts.at(kStartTextId).text = std::to_wstring(remainingSeconds);
        }
    }

    GetRenderer()->Render(nullptr, _objects, UI::Texts(), _sceneTexts);
}
