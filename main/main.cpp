#include <windows.h>
#include <memory>

#include "main.h"
#include "Input.h"
#include "Renderer.h"
#include "Scene/MainScene.h"
#include "UI.h"

#include <mmsystem.h>

#include <chrono>
#include <string>
#include <thread>

#pragma comment(lib, "winmm.lib")

std::mt19937 random_engine{ std::random_device{}() };
GameState gGameState;
UINT vsync = 0;

namespace {
    constexpr UINT kInitialWidth = 960;
    constexpr UINT kInitialHeight = 540;
    constexpr auto kTargetFrameDuration = std::chrono::microseconds(1'000'000 / 240);
    constexpr auto kSleepSafetyMargin = std::chrono::milliseconds(1);
    Renderer gRenderer;

    class TimerResolutionScope {
        public:
            TimerResolutionScope() : _isEnabled(timeBeginPeriod(1) == TIMERR_NOERROR) {}

            ~TimerResolutionScope() {
                if (_isEnabled) {
                    timeEndPeriod(1);
                }
            }

            TimerResolutionScope(const TimerResolutionScope&) = delete;
            TimerResolutionScope& operator=(const TimerResolutionScope&) = delete;

        private:
            bool _isEnabled = false;
    };

    void WaitUntilFrameDeadline(std::chrono::steady_clock::time_point deadline) {
        for (;;) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return;
            }

            const auto remaining = deadline - now;
            if (remaining > kSleepSafetyMargin) {
                // 긴 구간은 sleep으로 양보하고, 마지막 1ms는 아래 yield로 정밀하게 맞춘다.
                std::this_thread::sleep_for(remaining - kSleepSafetyMargin);
            }
            else {
                std::this_thread::yield();
            }
        }
    }

    LRESULT CALLBACK WindowProc(_In_ HWND window, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
        switch (message) {
            case WM_KEYDOWN:
                Input::SetKeyDown(static_cast<std::uint32_t>(wParam));
                return 0;
            case WM_KEYUP:
                Input::SetKeyUp(static_cast<std::uint32_t>(wParam));
                return 0;
            case WM_KILLFOCUS:
                // 포커스를 잃는 동안 놓인 키의 WM_KEYUP은 이 창에 오지 않을 수 있다.
                Input::Clear();
                return 0;
            case WM_SIZE:
                gRenderer.Resize(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(window, message, wParam, lParam);
    }
}

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int showCommand) {
    TimerResolutionScope timerResolution;
    constexpr wchar_t className[] = L"DirectXMovingRectangleWindow";
    const WNDCLASS windowClass{ CS_HREDRAW | CS_VREDRAW, WindowProc, 0, 0, instance,
        nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, className };
    if (!RegisterClass(&windowClass)) return 1;

    RECT rect{ 0, 0, static_cast<LONG>(kInitialWidth), static_cast<LONG>(kInitialHeight) };

    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION, FALSE);

    HWND window = CreateWindowEx(0, className, L"DirectX 11 - Moving Rectangles (WASD)", WS_OVERLAPPED | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, instance, nullptr);

    if (!window || !gRenderer.Initialize(window, kInitialWidth, kInitialHeight)) return 1;

    ShowWindow(window, showCommand);

    if (!gRenderer.CreateTextFormat(TEXT_FORMAT_UI, L"Arial", 32.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR) ||
        !gRenderer.CreateTextFormat(TEXT_FORMAT_GAMEOVER, L"Arial", 32.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER) ||
        !gRenderer.CreateTextFormat(TEXT_FORMAT_FPS, L"Arial", 20.0f,
            DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR)) {
        return 1;
    }

    // 모든 씬 위에 항상 표시되는 전역 UI다.
    UI::CreateText(TEXT_UI_FPS, L"FPS: --", TEXT_FORMAT_FPS,
        UI::TextAnchor::TopRight, -10.0f, 10.0f, 160.0f, 32.0f);

    const POINT clientOrigin = gRenderer.ClientScreenOrigin();
    SceneData data;

	data.x = clientOrigin.x + 480.0f;
	data.y = clientOrigin.y + 270.0f;
	data.width = 40.0f;
	data.height = 40.0f;
	data.r = 0.80f;
    data.g = 0.20f;
    data.b = 0.20f;
    data.speed = 400.0f;
    SceneBase::SetInitialScene(std::make_unique<MainScene>(data), &gRenderer);

    MSG message{};
    auto fpsMeasureStart = std::chrono::steady_clock::now();
    auto nextFrameTime = fpsMeasureStart;
    UINT frameCount = 0;
    bool wasF1Down = false;
    while (message.message != WM_QUIT) {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
            continue;
        }
        const bool isF1Down = Input::IsDown(VK_F1);
        if (isF1Down && !wasF1Down) {
            vsync ^= 1;
        }
        wasF1Down = isF1Down;

        ++frameCount;
        const auto now = std::chrono::steady_clock::now();
        const float elapsedSeconds = std::chrono::duration<float>(now - fpsMeasureStart).count();
        if (elapsedSeconds >= 0.5f) {
            const UINT fps = static_cast<UINT>(frameCount / elapsedSeconds + 0.5f);
            UI::SetText(TEXT_UI_FPS, (L"FPS: " + std::to_wstring(fps)).c_str());
            fpsMeasureStart = now;
            frameCount = 0;
        }
        SceneBase::UpdateCurrentScene();

        // VSync를 끈 상태에서도 240 FPS 주기까지 정밀하게 대기한다.
        nextFrameTime += kTargetFrameDuration;
        WaitUntilFrameDeadline(nextFrameTime);

        // 한 프레임 이상 늦었을 때만 기준 시각을 재설정해 지연 누적을 막는다.
        const auto actualFrameEndTime = std::chrono::steady_clock::now();
        if (actualFrameEndTime - nextFrameTime > kTargetFrameDuration) {
            nextFrameTime = actualFrameEndTime;
        }
    }
    return 0;
}
