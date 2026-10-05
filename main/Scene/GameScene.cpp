
#include "GameScene.h"

#include "../TimerEvent.h"
#include "../Input.h"
#include "../UI.h"
#include "../main.h"

#include <cmath>
#include <random>
#include <string>

namespace {
    constexpr int kMoveLeft = 0b01;
    constexpr int kMoveUp = 0b10;
    constexpr float kWindowMoveSpeed = 100.0f;
    // 최소화, 디버거 중단 등으로 큰 시간이 한 번에 반영되는 것을 방지한다.
    constexpr float kMaximumDeltaSeconds = 0.1f;
    constexpr UI::TextId kCountdownTextId = 100;
    constexpr float kCountdownSeconds = 3.0f;
}

static void UpdateFromWASD(Object::Rectangle& rect, float deltaSeconds, UINT screenWidth, UINT screenHeight, POINT clientOrigin, float scale) {
    float xDirection = 0.0f;
    float yDirection = 0.0f;

    if (Input::IsDown('A')) {
        xDirection -= 1.0f;
    }
    if (Input::IsDown('D')) {
        xDirection += 1.0f;
    }
    if (Input::IsDown('W')) {
        yDirection -= 1.0f;
    }
    if (Input::IsDown('S')) {
        yDirection += 1.0f;
    }

    // Convert the scale-adjusted visible edges back into world coordinates.
    // The calculation is the inverse of the renderer's center-based projection.
    const float safeScale = std::max(0.01f, scale);
    const float centerX = screenWidth * 0.5f;
    const float centerY = screenHeight * 0.5f;
    const float left = clientOrigin.x + centerX +
        (rect.width * safeScale * 0.5f - centerX) / safeScale;
    const float top = clientOrigin.y + centerY +
        (rect.height * safeScale * 0.5f - centerY) / safeScale;
    const float right = clientOrigin.x + centerX +
        (static_cast<float>(screenWidth) - rect.width * safeScale * 0.5f - centerX) / safeScale;
    const float bottom = clientOrigin.y + centerY +
        (static_cast<float>(screenHeight) - rect.height * safeScale * 0.5f - centerY) / safeScale;
    const float scaledSpeed = rect.speed * safeScale;
    rect.x = std::clamp(rect.x + xDirection * scaledSpeed * deltaSeconds, left, std::max(left, right));
    rect.y = std::clamp(rect.y + yDirection * scaledSpeed * deltaSeconds, top, std::max(top, bottom));
}

bool GameScene::IsOverlapping(const Object::Rectangle& first, const Object::Rectangle& second) {
    const float xDistance = std::abs(first.x - second.x);
    const float yDistance = std::abs(first.y - second.y);
    const float halfWidthSum = (first.width + second.width) * 0.5f;
    const float halfHeightSum = (first.height + second.height) * 0.5f;

    return xDistance <= halfWidthSum && yDistance <= halfHeightSum;
}

GameState& GameScene::GetGameState() {
    return gGameState;
}

void GameScene::Update() {
    Renderer* renderer = GetRenderer();
    if (!renderer) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const float elapsedSeconds = std::chrono::duration<float>(now - _previousTime).count();
    const float deltaSeconds = std::clamp(elapsedSeconds, 0.0f, kMaximumDeltaSeconds);
    RECT windowRect;

    if (!_isPlayerInitialized) {
        return;
    }

    _previousTime = now;
    if (_isCountingDown) {
        const float countdownElapsed = std::chrono::duration<float>(now - _countdownStart).count();
        if (countdownElapsed >= kCountdownSeconds) {
            StartGameplay();
        }
        else {
            const int remainingSeconds = static_cast<int>(std::ceil(kCountdownSeconds - countdownElapsed));
            _sceneTexts.at(kCountdownTextId).text = std::to_wstring(remainingSeconds);
        }

        renderer->Render(&gGameState, _object, UI::Texts(), _sceneTexts, vsync);
        return;
    }

    const bool isF5Down = Input::IsDown(VK_F5);
    if (!gGameState._isAlive && isF5Down && !_wasF5Down) {
        // 새 인스턴스를 요청한다. 현재 씬은 Update가 끝난 후 SceneBase가 파기한다.
        SceneBase::RequestSceneChange(std::make_unique<GameScene>(_initialData));
    }
    _wasF5Down = isF5Down;

    if (_transitionTime > 0.0f) {
        _transitionTime = std::max(0.0f, _transitionTime - deltaSeconds);

        const float progress = 1.0f - _transitionTime / _transitionDuration;
        const float scale = _transitionStartScale +
            (_transitionScale - _transitionStartScale) * progress;
        SetScale(scale);
    }
    if (!_pause) {
        if (gGameState._isAlive) {
            GetWindowRect(renderer->Window(), &windowRect);

            const LONG virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
            const LONG virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
            const LONG virtualRight = virtualLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN);
            const LONG virtualBottom = virtualTop + GetSystemMetrics(SM_CYVIRTUALSCREEN);
            const LONG windowWidth = windowRect.right - windowRect.left;
            const LONG windowHeight = windowRect.bottom - windowRect.top;
            const LONG maxLeft = std::max(virtualLeft, virtualRight - windowWidth);
            const LONG maxTop = std::max(virtualTop, virtualBottom - windowHeight);

            const float actualWindowX = static_cast<float>(windowRect.left);
            const float actualWindowY = static_cast<float>(windowRect.top);
            if (!_isWindowPositionInitialized ||
                std::abs(actualWindowX - _windowX) > 1.0f ||
                std::abs(actualWindowY - _windowY) > 1.0f) {
                // 사용자가 창을 직접 옮겼다면 누적 위치를 실제 위치에 맞춘다.
                _windowX = actualWindowX;
                _windowY = actualWindowY;
                _isWindowPositionInitialized = true;
            }

            const float xDirection = (_vector & kMoveLeft) ? -1.0f : 1.0f;
            const float yDirection = (_vector & kMoveUp) ? -1.0f : 1.0f;
            _windowX += xDirection * kWindowMoveSpeed * deltaSeconds;
            _windowY += yDirection * kWindowMoveSpeed * deltaSeconds;

            if (_windowX <= virtualLeft) {
                _windowX = static_cast<float>(virtualLeft);
                _vector &= ~kMoveLeft;
            }
            else if (_windowX >= maxLeft) {
                _windowX = static_cast<float>(maxLeft);
                _vector |= kMoveLeft;
            }
            if (_windowY <= virtualTop) {
                _windowY = static_cast<float>(virtualTop);
                _vector &= ~kMoveUp;
            }
            else if (_windowY >= maxTop) {
                _windowY = static_cast<float>(maxTop);
                _vector |= kMoveUp;
            }

            SetWindowPos(renderer->Window(), nullptr,
                static_cast<LONG>(std::lround(_windowX)),
                static_cast<LONG>(std::lround(_windowY)), 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            UpdateFromWASD(gGameState.player, deltaSeconds, renderer->Width(), renderer->Height(), renderer->ClientScreenOrigin(), _scale);
        }

        for (auto& [id, objectPointer] : _object) {
            // 제거는 순회가 끝난 뒤 처리한다. erase를 즉시 수행하면 iterator가 무효화된다.
            bool shouldRemove = false;
            Object::Rectangle& object = *objectPointer;

            if (object.controlAI) {
                shouldRemove = object.controlAI(object, deltaSeconds, gGameState, _scale);
            }
            if (!shouldRemove && gGameState._isAlive && IsOverlapping(gGameState.player, object) && object.impact) {
                switch (object.impact(*this, object, gGameState)) {
                    case Object::RemoveTarget::Object:
                        shouldRemove = true;
                        break;
                    case Object::RemoveTarget::Player:
                        ApplyDamage(gGameState, gGameState._life.Get());
                        break;
                    case Object::RemoveTarget::RandomEnemy:
                        shouldRemove = true;
                        RemoveRandomEnemy();
                        break;
                }
                if (gGameState._life.Get() == 0) {
                    gGameState._isAlive = false;
                }
            }

            if (shouldRemove) {
                QueueObjectRemoval(id);
            }
        }
        ProcessPendingRemovals();
    }
    _timers.Update(*this, deltaSeconds);
    renderer->Render(&gGameState, _object, UI::Texts(), _sceneTexts, vsync);
}

void GameScene::Initialize(_In_ Renderer* renderer) {
    SceneBase::Initialize(renderer);
	const SceneData& data = _initialData;
	gGameState = {};
	gGameState.player.x = data.x;
	gGameState.player.y = data.y;
	gGameState.player.width = data.width;
	gGameState.player.height = data.height;
	gGameState.player.speed = data.speed;
	gGameState.player.red = data.r;
	gGameState.player.green = data.g;
	gGameState.player.blue = data.b;
	gGameState._isAlive = true;
    _isPlayerInitialized = true;
    _isWindowPositionInitialized = false;
    GetRenderer()->SetScale(_scale);
    _previousTime = std::chrono::steady_clock::now();

    // ClearTexts를 호출하지 않는다. FPS는 씬과 무관한 전역 UI이기 때문이다.
    if (!UI::CreateText(TEXT_UI_LIFE, L"", TEXT_FORMAT_UI,
        UI::TextAnchor::TopLeft, 10.0f, 10.0f, 240.0f, 60.0f)) {
        UI::SetText(TEXT_UI_LIFE, L"");
    }
    UpdateLifeText(gGameState);
    if (!UI::CreateText(TEXT_UI_GAMEOVER, L"", TEXT_FORMAT_GAMEOVER,
        UI::TextAnchor::Center, 0.0f, 0.0f, 400.0f, 100.0f)) {
        UI::SetText(TEXT_UI_GAMEOVER, L"");
    }

    _sceneTexts.clear();
    _sceneTexts.emplace(kCountdownTextId, UI::TextDrawRequest{
        L"3", TEXT_FORMAT_GAMEOVER, UI::TextAnchor::Center,
        0.0f, 0.0f, 500.0f, 80.0f
    });
    _isCountingDown = true;
    _countdownStart = _previousTime;
}

void GameScene::StartGameplay() {
    _isCountingDown = false;
    _sceneTexts.erase(kCountdownTextId);
    AddOnce(2.0f, TimerCallbackList::SpawnEnemy);
    AddRepeatAfter(2.0f, 5.0f, TimerCallbackList::SpawnEnemy);
    AddRepeat(30.0f, TimerCallbackList::MapScale);
}

void GameScene::CreateObject(SceneData data, Object::Type type, Object::ControlAI func, Object::ImpactFunc impact) {
    auto obj = std::make_unique<Object::Rectangle>();
    const Object::ObjectId id = _nextObjectId++;

    obj->id = id;
    obj->x = data.x;
    obj->y = data.y;
    obj->width = data.width;
    obj->height = data.height;
    obj->speed = data.speed;
    obj->red = data.r;
    obj->green = data.g;
    obj->blue = data.b;
    obj->type = type;
    obj->controlAI = func;
    obj->impact = impact;
	_object.emplace(id, std::move(obj));
    if (type == Object::Type::Enemy) {
        _enemy.push_back(id);
    }
}

void GameScene::RemoveRandomEnemy() {
    if (_enemy.empty()) {
        return;
    }

    std::uniform_int_distribution<std::size_t> distribution(0, _enemy.size() - 1);
    QueueObjectRemoval(_enemy[distribution(random_engine)]);
}

void GameScene::QueueObjectRemoval(Object::ObjectId id) {
    if (std::find(_pendingRemovals.begin(), _pendingRemovals.end(), id) == _pendingRemovals.end()) {
        _pendingRemovals.push_back(id);
    }
}

void GameScene::ProcessPendingRemovals() {
    for (Object::ObjectId id : _pendingRemovals) {
        _object.erase(id);
        std::erase(_enemy, id);
    }

    _pendingRemovals.clear();
}

void GameScene::SetScale(float scale) {
    _scale = std::max(0.01f, scale);
    if (Renderer* renderer = GetRenderer()) {
        renderer->SetScale(_scale);
    }
}

void GameScene::SetScaleTrans(float toScale, float sec) {
    _transitionScale = std::max(0.01f, toScale);
    _transitionStartScale = _scale;
    _transitionDuration = std::max(0.0f, sec);
    _transitionTime = _transitionDuration;

    if (_transitionDuration <= 0.0f) {
        SetScale(_transitionScale);
    }
}

bool GameScene::CreateSceneText(UI::TextId id, _In_opt_z_ const wchar_t* text, UI::TextFormatId formatId, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height) {
    return _sceneTexts.emplace(id, UI::TextDrawRequest{ text ? text : L"", formatId, anchor, offsetX, offsetY, width, height }).second;
}

bool GameScene::SetSceneText(UI::TextId id, _In_opt_z_ const wchar_t* text) {
    const auto iterator = _sceneTexts.find(id);
    if (iterator == _sceneTexts.end()) {
        return false;
    }

    iterator->second.text = text ? text : L"";
    return true;
}

bool GameScene::SetSceneTextLayout(UI::TextId id, UI::TextAnchor anchor, float offsetX, float offsetY, float width, float height) {
    const auto iterator = _sceneTexts.find(id);
    if (iterator == _sceneTexts.end()) {
        return false;
    }

    iterator->second.anchor = anchor;
    iterator->second.offsetX = offsetX;
    iterator->second.offsetY = offsetY;
    iterator->second.width = width;
    iterator->second.height = height;
    return true;
}

void GameScene::RemoveSceneText(UI::TextId id) {
    _sceneTexts.erase(id);
}

TimerId GameScene::AddOnce(float delaySeconds, TimerCallback callback) {
    return _timers.AddOnce(delaySeconds, callback);
}

TimerId GameScene::AddRepeat(float intervalSeconds, TimerCallback callback) {
    return _timers.AddRepeat(intervalSeconds, callback);
}

TimerId GameScene::AddRepeatAfter(float delaySeconds, float intervalSeconds, TimerCallback callback) {
    return _timers.AddRepeatAfter(delaySeconds, intervalSeconds, callback);
}

void GameScene::CancelTimer(TimerId id) {
    _timers.Cancel(id);
}
