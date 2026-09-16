#include <windows.h>
#include <memory>

#include "main.h"
#include "Renderer.h"
#include "UI.h"

std::mt19937 random_engine{ std::random_device{}() };
GameState gGameState;
std::unique_ptr<Scene> gScene;

namespace {
    constexpr UINT kInitialWidth = 960;
    constexpr UINT kInitialHeight = 540;
    Renderer gRenderer;
    bool gIsMovingOrSizing = false;

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
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

    const POINT clientOrigin = gRenderer.ClientScreenOrigin();
    
    gScene = std::make_unique<Scene>();

    SceneData data;

	data.x = clientOrigin.x + 480.0f;
	data.y = clientOrigin.y + 270.0f;
	data.width = 40.0f;
	data.height = 40.0f;
	data.r = 0.80f;
	data.g = 0.20f;
    data.b = 0.20f;
    data.speed = 400.0f;
    gScene->Initialize(data, &gRenderer);
    if (!gRenderer.CreateTextFormat(TEXT_FORMAT_UI, L"Arial", 32.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR) ||
        !gRenderer.CreateTextFormat(TEXT_FORMAT_GAMEOVER, L"Arial", 32.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER)) {
        return 1;
    }
    UI::CreateText(
        TEXT_UI_LIFE,
        L"",
        TEXT_FORMAT_UI,
        UI::TextAnchor::TopLeft,
        10.0f,
        10.0f,
        240.0f,
        60.0f
    );
    UpdateLifeText(gGameState);
    UI::CreateText(
        TEXT_UI_GAMEOVER,
        L"",
        TEXT_FORMAT_GAMEOVER,
        UI::TextAnchor::Center,
        0.0f,
        0.0f,
        400.0f,
        100.0f
    );
    MSG message{};
    while (message.message != WM_QUIT) {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
            continue;
        }
        gScene->Update();
    }
    return 0;
}
