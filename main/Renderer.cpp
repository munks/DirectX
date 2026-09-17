#include "Renderer.h"
#include "GameState.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d2d1.h>
#include <dwrite.h>

#include <array>
#include <cwchar>
#include <cstring>
#include <utility>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

struct Vertex {
    float position[2];
};

static_assert(sizeof(Object::RectangleConstants) == 48,
    "RectangleConstants must match the HLSL constant-buffer layout.");

static bool CompileShader(const char* source, const char* entryPoint, const char* target, ComPtr<ID3DBlob>& byteCode) {
    ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr,
        entryPoint, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, byteCode.GetAddressOf(), errors.GetAddressOf());
    return SUCCEEDED(result);
}

template <typename RectangleType>
static Object::RectangleConstants BuildRectangleConstants(const RectangleType& rectangle,
    POINT clientOrigin, UINT viewportWidth, UINT viewportHeight, float scale) {
    // 게임 좌표는 데스크톱 절대 좌표다. 렌더링 직전에 클라이언트 좌표로 옮긴 뒤
    // 뷰포트 중심을 기준으로 확대/축소한다.
    const float localX = rectangle.x - clientOrigin.x;
    const float localY = rectangle.y - clientOrigin.y;
    const float projectedX = viewportWidth * 0.5f + (localX - viewportWidth * 0.5f) * scale;
    const float projectedY = viewportHeight * 0.5f + (localY - viewportHeight * 0.5f) * scale;

    return {
        {projectedX, projectedY}, {static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)},
        {rectangle.width * scale, rectangle.height * scale}, {0.0f, 0.0f},
        {rectangle.red, rectangle.green, rectangle.blue}, 0.0f
    };
}

static D2D1_RECT_F GetTextRectangle(const UI::TextDrawRequest& text, float viewportWidth, float viewportHeight) {
    float anchorX = 0.0f;
    float anchorY = 0.0f;
    float alignmentX = 0.0f;
    float alignmentY = 0.0f;

    switch (text.anchor) {
        case UI::TextAnchor::TopLeft:
            break;
        case UI::TextAnchor::Top:
            anchorX = viewportWidth * 0.5f;
            alignmentX = 0.5f;
            break;
        case UI::TextAnchor::TopRight:
            anchorX = viewportWidth;
            alignmentX = 1.0f;
            break;
        case UI::TextAnchor::Left:
            anchorY = viewportHeight * 0.5f;
            alignmentY = 0.5f;
            break;
        case UI::TextAnchor::Center:
            anchorX = viewportWidth * 0.5f;
            anchorY = viewportHeight * 0.5f;
            alignmentX = 0.5f;
            alignmentY = 0.5f;
            break;
        case UI::TextAnchor::Right:
            anchorX = viewportWidth;
            anchorY = viewportHeight * 0.5f;
            alignmentX = 1.0f;
            alignmentY = 0.5f;
            break;
        case UI::TextAnchor::BottomLeft:
            anchorY = viewportHeight;
            alignmentY = 1.0f;
            break;
        case UI::TextAnchor::Bottom:
            anchorX = viewportWidth * 0.5f;
            anchorY = viewportHeight;
            alignmentX = 0.5f;
            alignmentY = 1.0f;
            break;
        case UI::TextAnchor::BottomRight:
            anchorX = viewportWidth;
            anchorY = viewportHeight;
            alignmentX = 1.0f;
            alignmentY = 1.0f;
            break;
    }

    // anchor는 기준점, offset은 기준점으로부터의 이동량이다.
    const float left = anchorX + text.offsetX - text.width * alignmentX;
    const float top = anchorY + text.offsetY - text.height * alignmentY;
    return D2D1::RectF(left, top, left + text.width, top + text.height);
}

Renderer::~Renderer() {
    Release();
}

void Renderer::Release() {
    if (context_) context_->ClearState();

    constantBuffer_.Reset();
    vertexBuffer_.Reset();
    inputLayout_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    textBrush_.Reset();
    d2dRenderTarget_.Reset();
    renderTarget_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    d2dFactory_.Reset();
    textFormats_.clear();
    writeFactory_.Reset();
    window_ = nullptr;
}

POINT Renderer::ClientScreenOrigin() const {
    POINT origin{};
    if (window_) {
        ClientToScreen(window_, &origin);
    }
    return origin;
}

bool Renderer::CreateTextFormat(UI::TextFormatId id, _In_z_ const wchar_t* fontFamily, float fontSize,
    DWRITE_TEXT_ALIGNMENT textAlignment, DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment) {
    if (!writeFactory_ || !fontFamily || fontSize <= 0.0f || textFormats_.contains(id)) {
        return false;
    }

    ComPtr<IDWriteTextFormat> textFormat;
    if (FAILED(writeFactory_->CreateTextFormat(
        fontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"ko-kr",
        textFormat.GetAddressOf()))) {
        return false;
    }

    if (FAILED(textFormat->SetTextAlignment(textAlignment)) ||
        FAILED(textFormat->SetParagraphAlignment(paragraphAlignment))) {
        return false;
    }

    textFormats_.emplace(id, std::move(textFormat));
    return true;
}

IDWriteTextFormat* Renderer::FindTextFormat(UI::TextFormatId id) const {
    const auto iterator = textFormats_.find(id);
    return iterator != textFormats_.end() ? iterator->second.Get() : nullptr;
}

void Renderer::DrawTexts(const UI::TextDrawRequests& texts) {
    for (const auto& entry : texts) {
        const UI::TextDrawRequest& text = entry.second;
        IDWriteTextFormat* textFormat = FindTextFormat(text.formatId);
        if (text.text.empty() || !textFormat) {
            continue;
        }

        d2dRenderTarget_->DrawText(
            text.text.c_str(),
            static_cast<UINT32>(text.text.size()),
            textFormat,
            GetTextRectangle(text, static_cast<float>(width_), static_cast<float>(height_)),
            textBrush_.Get()
        );
    }
}

bool Renderer::CreateRenderTarget() {
    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())))) return false;

    const HRESULT result = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTarget_.GetAddressOf());
    return SUCCEEDED(result);
}

bool Renderer::CreateDirect2DTarget() {
    ComPtr<IDXGISurface> surface;
    if (FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf())))) {
        return false;
    }

    const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    const HRESULT targetResult = d2dFactory_->CreateDxgiSurfaceRenderTarget(surface.Get(), &properties, d2dRenderTarget_.GetAddressOf());
    if (FAILED(targetResult)) {
        return false;
    }

    return SUCCEEDED(d2dRenderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White), textBrush_.GetAddressOf()));
}

bool Renderer::Initialize(_In_ HWND window, UINT initialWidth, UINT initialHeight) {
    window_ = window;
    width_ = initialWidth;
    height_ = initialHeight;

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf())))) {
        Release();
        return false;
    }
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf()))) {
        Release();
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.Width = initialWidth;
    swapChainDesc.BufferDesc.Height = initialHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel{};
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, swapChain_.GetAddressOf(), device_.GetAddressOf(), &featureLevel, context_.GetAddressOf())) ||
        !CreateRenderTarget() || !CreateDirect2DTarget()) {
        Release();
        return false;
    }

    constexpr char shaderSource[] = R"(
cbuffer RectangleData : register(b0)
{
    float2 position;
    float2 viewportSize;
    float2 rectangleSize;
    float2 sizePadding;
    float3 rectangleColor;
    float colorPadding;
};
struct VertexInput { float2 position : POSITION; };
struct PixelInput { float4 position : SV_POSITION; float3 color : COLOR; };
PixelInput VSMain(VertexInput input) {
    PixelInput output;
    float2 pixelPosition = input.position * rectangleSize + position;
    output.position = float4(pixelPosition.x / viewportSize.x * 2.0f - 1.0f,
                             1.0f - pixelPosition.y / viewportSize.y * 2.0f, 0.0f, 1.0f);
    output.color = rectangleColor;
    return output;
}
float4 PSMain(PixelInput input) : SV_TARGET { return float4(input.color, 1.0f); }
)";

    ComPtr<ID3DBlob> vertexByteCode;
    ComPtr<ID3DBlob> pixelByteCode;
    const bool compiled = CompileShader(shaderSource, "VSMain", "vs_5_0", vertexByteCode) &&
        CompileShader(shaderSource, "PSMain", "ps_5_0", pixelByteCode);
    if (!compiled) {
        Release();
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    const HRESULT vertexResult = device_->CreateVertexShader(vertexByteCode->GetBufferPointer(), vertexByteCode->GetBufferSize(), nullptr, vertexShader_.GetAddressOf());
    const HRESULT layoutResult = device_->CreateInputLayout(inputElements, ARRAYSIZE(inputElements), vertexByteCode->GetBufferPointer(), vertexByteCode->GetBufferSize(), inputLayout_.GetAddressOf());
    const HRESULT pixelResult = device_->CreatePixelShader(pixelByteCode->GetBufferPointer(), pixelByteCode->GetBufferSize(), nullptr, pixelShader_.GetAddressOf());
    if (FAILED(vertexResult) || FAILED(layoutResult) || FAILED(pixelResult)) { Release(); return false; }

    const std::array<Vertex, 4> vertices = {{
        {{-0.5f, -0.5f}}, {{0.5f, -0.5f}},
        {{-0.5f, 0.5f}}, {{0.5f, 0.5f}},
    }};
    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = sizeof(vertices);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices.data();
    if (FAILED(device_->CreateBuffer(&vertexBufferDesc, &vertexData, vertexBuffer_.GetAddressOf()))) { Release(); return false; }

    D3D11_BUFFER_DESC constantBufferDesc{};
    constantBufferDesc.ByteWidth = sizeof(Object::RectangleConstants);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device_->CreateBuffer(&constantBufferDesc, nullptr, constantBuffer_.GetAddressOf()))) { Release(); return false; }
    return true;
}

void Renderer::Resize(UINT width, UINT height) {
    if (!swapChain_ || width == 0 || height == 0) return;
    textBrush_.Reset();
    d2dRenderTarget_.Reset();
    renderTarget_.Reset();
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    if (SUCCEEDED(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)) &&
        CreateRenderTarget() && CreateDirect2DTarget()) {
        width_ = width;
        height_ = height;
    }
}

void Renderer::Render(_In_opt_ const GameState* gameState, const std::unordered_map<Object::ObjectId, std::unique_ptr<Object::Rectangle>>& rectangles, const UI::TextDrawRequests& globalTexts, const UI::TextDrawRequests& sceneTexts) {
    if (!renderTarget_) return;
    constexpr float clearColor[] = { 0.04f, 0.06f, 0.10f, 1.0f };
    context_->ClearRenderTargetView(renderTarget_.Get(), clearColor);
    context_->OMSetRenderTargets(1, renderTarget_.GetAddressOf(), nullptr);
    const D3D11_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, 1.0f };
    context_->RSSetViewports(1, &viewport);
    const UINT stride = sizeof(Vertex), offset = 0;
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    const POINT clientOrigin = ClientScreenOrigin();

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (gameState && gameState->_isAlive && SUCCEEDED(context_->Map(constantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        *static_cast<Object::RectangleConstants*>(mapped.pData) =
            BuildRectangleConstants(gameState->player, clientOrigin, width_, height_, scale_);
        context_->Unmap(constantBuffer_.Get(), 0);
        context_->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
        context_->Draw(4, 0);
    }
    for (const auto& [id, rectangle] : rectangles) {
        if (!rectangle) {
            continue;
        }
        // Map 실패 시에는 이전 상수 버퍼를 재사용하지 않고 해당 객체만 건너뛴다.
        if (FAILED(context_->Map(constantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            continue;
        }
        *static_cast<Object::RectangleConstants*>(mapped.pData) =
            BuildRectangleConstants(*rectangle, clientOrigin, width_, height_, scale_);
        context_->Unmap(constantBuffer_.Get(), 0);
        context_->VSSetConstantBuffers(0, 1, constantBuffer_.GetAddressOf());
        context_->Draw(4, 0);
    }
    if ((!globalTexts.empty() || !sceneTexts.empty()) && d2dRenderTarget_ && textBrush_) {
        context_->OMSetRenderTargets(0, nullptr, nullptr);
        context_->Flush();
        d2dRenderTarget_->BeginDraw();
        DrawTexts(globalTexts);
        DrawTexts(sceneTexts);
        d2dRenderTarget_->EndDraw();
    }
    swapChain_->Present(1, 0);
}
