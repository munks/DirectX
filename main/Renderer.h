#pragma once

#include <windows.h>
#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl/client.h>

#include "Rectangle.h"

struct GameState;

namespace UI {
    using TextId = std::uint64_t;
    using TextFormatId = std::uint64_t;

    enum class TextAnchor {
        TopLeft,
        TopRight,
        Top,
        Center,
        BottomLeft,
        BottomRight,
        Bottom,
        Left,
        Right
    };

    struct TextDrawRequest {
        std::wstring text;
        TextFormatId formatId = 0;
        TextAnchor anchor = TextAnchor::TopLeft;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    using TextDrawRequests = std::unordered_map<TextId, TextDrawRequest>;
}

class Renderer {
    public:
        Renderer() = default;
        ~Renderer();
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize(HWND window, UINT initialWidth, UINT initialHeight);
        void Resize(UINT width, UINT height);
        void Render(const GameState* player, const std::unordered_map<Object::ObjectId, std::unique_ptr<Object::Rectangle>>& rectangles, const UI::TextDrawRequests& globalTexts, const UI::TextDrawRequests& sceneTexts);
        void SetScale(float scale) { scale_ = scale; }
        bool CreateTextFormat(UI::TextFormatId id, const wchar_t* fontFamily, float fontSize, DWRITE_TEXT_ALIGNMENT textAlignment, DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment);
        IDWriteTextFormat* FindTextFormat(UI::TextFormatId id) const;
        IDWriteFactory* WriteFactory() const { return writeFactory_.Get(); }

        UINT Width() const { return width_; }
        UINT Height() const { return height_; }
        POINT ClientScreenOrigin() const;
		HWND Window() const { return window_; }

    private:
        bool CreateRenderTarget();
        bool CreateDirect2DTarget();
        void DrawTexts(const UI::TextDrawRequests& texts);
        void Release();

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
        Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
        std::unordered_map<UI::TextFormatId, Microsoft::WRL::ComPtr<IDWriteTextFormat>> textFormats_;
        Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> d2dRenderTarget_;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
        HWND window_ = nullptr;
        UINT width_ = 0;
        UINT height_ = 0;
        float scale_ = 1.0f;
};
