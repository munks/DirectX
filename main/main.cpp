#include <windows.h>
#include <memory>

#include "main.h"
#include "Renderer.h"
#include "Scene/MainScene.h"
#include "UI.h"

#include <chrono>
#include <string>

std::mt19937 random_engine{ std::random_device{}() };
GameState gGameState;

namespace {
    constexpr UINT kInitialWidth = 960;
    constexpr UINT kInitialHeight = 540;
    Renderer gRenderer;
    bool gIsMovingOrSizing = false;

    LRESULT CALLBACK WindowProc(_In_ HWND window, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam) {
        switch (message) {
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
    UINT frameCount = 0;
    while (message.message != WM_QUIT) {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
            continue;
        }
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
    }
    return 0;
}
