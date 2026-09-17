#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>
#include "SceneBase.h"
#include "../Renderer.h"
#include "../TimerManager.h"

struct GameState;

class GameScene final : public SceneBase {
    public:
        explicit GameScene(SceneData initialData) : _initialData(initialData) {}
        GameScene(const GameScene&) = delete;
        GameScene& operator=(const GameScene&) = delete;

        void Initialize(_In_ Renderer* renderer) override;
        void Update() override;
        void CreateObject(SceneData data, Object::Type type = Object::Type::Undefined, Object::ControlAI func = nullptr, Object::ImpactFunc impact = nullptr);
        void RemoveRandomEnemy();
        void SetScale(float scale);
        void SetScaleTrans(float toScale, float sec);
        bool CreateSceneText(UI::TextId id, _In_opt_z_ const wchar_t* text, UI::TextFormatId formatId, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height);
        bool SetSceneText(UI::TextId id, _In_opt_z_ const wchar_t* text);
        bool SetSceneTextLayout(UI::TextId id, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height);
        void RemoveSceneText(UI::TextId id);
        float GetScale() const { return _scale; }
        TimerId AddOnce(float delaySeconds, TimerCallback callback);
        TimerId AddRepeat(float intervalSeconds, TimerCallback callback);
        TimerId AddRepeatAfter(float delaySeconds, float intervalSeconds, TimerCallback callback);
        void CancelTimer(TimerId id);
        GameState& GetGameState();
        Renderer* GetRenderer() const { return SceneBase::GetRenderer(); }
    private:
        SceneData _initialData;
        bool IsOverlapping(const Object::Rectangle& first, const Object::Rectangle& second);
        void QueueObjectRemoval(Object::ObjectId id);
        void ProcessPendingRemovals();

        std::unordered_map<Object::ObjectId, std::unique_ptr<Object::Rectangle>> _object;
        std::vector<Object::ObjectId> _enemy;
        std::vector<Object::ObjectId> _pendingRemovals;
        Object::ObjectId _nextObjectId = 1;
        UI::TextDrawRequests _sceneTexts;
        bool _isPlayerInitialized = false;
        std::chrono::steady_clock::time_point _previousTime{};
        TimerManager _timers;
        int _vector = 0b00;
        float _scale = 1.0f;
        float _transitionStartScale = 1.0f;
        float _transitionTime = 0.0;
        float _transitionDuration = 0.0f;
        float _transitionScale = 1.0f;
        bool _pause = false;
        bool _wasF5Down = false;
        
};
