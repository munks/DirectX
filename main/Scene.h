#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Renderer.h"
#include "TimerManager.h"

struct GameState;

typedef struct SceneData {
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float speed;
} SceneData;

class Scene {
    public:
        Scene() = default;
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        void Initialize(SceneData data, Renderer* renderer);
        void Update();
        void CreateObject(SceneData data, Object::Type type = Object::Type::Undefined, Object::ControlAI func = nullptr, Object::ImpactFunc impact = nullptr);
        void RemoveRandomEnemy();
        void SetScale(float scale);
        void SetScaleTrans(float toScale, float sec);
        bool CreateSceneText(UI::TextId id, const wchar_t* text, UI::TextFormatId formatId, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height);
        bool SetSceneText(UI::TextId id, const wchar_t* text);
        bool SetSceneTextLayout(UI::TextId id, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height);
        void RemoveSceneText(UI::TextId id);
        float GetScale() { return _scale; }
        TimerId AddOnce(float delaySeconds, TimerCallback callback);
        TimerId AddRepeat(float intervalSeconds, TimerCallback callback);
        TimerId AddRepeatAfter(float delaySeconds, float intervalSeconds, TimerCallback callback);
        void CancelTimer(TimerId id);
        GameState& GetGameState();
        Renderer* GetRenderer() {
            return _renderer;
        }
    private:
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
        Renderer* _renderer = nullptr;
        TimerManager _timers;
        int _vector = 0b00;
        float _scale = 1.0f;
        float _transitionStartScale = 1.0f;
        float _transitionTime = 0.0;
        float _transitionDuration = 0.0f;
        float _transitionScale = 1.0f;
        bool _pause = false;
        
};
