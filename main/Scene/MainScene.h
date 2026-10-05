#pragma once

#include "SceneBase.h"

#include <unordered_map>

#include "../Renderer.h"

class MainScene final : public SceneBase {
    public:
        explicit MainScene(SceneData gameData) : _gameData(gameData) {}

    protected:
        void Initialize(_In_ Renderer* renderer) override;
        void Update() override;

    private:
        SceneData _gameData;
        std::unordered_map<Object::ObjectId, std::unique_ptr<Object::Rectangle>> _objects;
        UI::TextDrawRequests _sceneTexts;
        bool _wasSpaceDown = false;
};
