#include "MainScene.h"

#include "GameScene.h"
#include "../Input.h"
#include "../main.h"
#include "../UI.h"

namespace {
    constexpr UI::TextId kStartTextId = 100;
}

void MainScene::Initialize(_In_ Renderer* renderer) {
    SceneBase::Initialize(renderer);
    _sceneTexts.clear();
    _sceneTexts.emplace(kStartTextId, UI::TextDrawRequest{
        L"Press Space to Start", TEXT_FORMAT_GAMEOVER, UI::TextAnchor::Center,
        0.0f, 0.0f, 500.0f, 80.0f
    });
    _wasSpaceDown = false;
}

void MainScene::Update() {
    const bool isSpaceDown = Input::IsDown(VK_SPACE);
    if (isSpaceDown && !_wasSpaceDown) {
        SceneBase::RequestSceneChange(std::make_unique<GameScene>(_gameData));
    }
    _wasSpaceDown = isSpaceDown;

    GetRenderer()->Render(nullptr, _objects, UI::Texts(), _sceneTexts, vsync);
}
