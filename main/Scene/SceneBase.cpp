#include "SceneBase.h"

#include "../Renderer.h"

void SceneBase::Initialize(_In_ Renderer* renderer) {
    _renderer = renderer;
}

void SceneBase::SetInitialScene(std::unique_ptr<SceneBase> scene, _In_ Renderer* renderer) {
    _sceneRenderer = renderer;
    _currentScene = std::move(scene);
    if (_currentScene) {
        _currentScene->Initialize(_sceneRenderer);
    }
}

void SceneBase::RequestSceneChange(std::unique_ptr<SceneBase> scene) {
    _pendingScene = std::move(scene);
}

void SceneBase::UpdateCurrentScene() {
    if (!_currentScene) {
        return;
    }

    _currentScene->Update();
    if (_pendingScene) {
        _currentScene = std::move(_pendingScene);
        _currentScene->Initialize(_sceneRenderer);
    }
}
