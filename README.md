# DirectX Moving Rectangles

DirectX 11, Direct2D, DirectWrite를 이용해 만든 Windows용 간단한 실시간 사각형 게임 프로젝트입니다. 플레이어 사각형을 WASD로 움직이고, 적·중립 오브젝트·타이머·UI 텍스트를 씬에서 관리합니다.

## 기능

- DirectX 11로 색상 사각형 렌더링
- Direct2D/DirectWrite 기반 텍스트 UI 렌더링
- WASD 플레이어 이동 및 화면 경계 처리
- 사각형 중심 좌표 기반 충돌 판정
- 적의 플레이어 추적 및 접촉 시 피해 처리
- 중립 오브젝트 접촉 시 임의 적 제거
- 일회성·반복·지연 반복 타이머
- 화면 중심 기준 스케일 전환 효과
- 텍스트와 텍스트 포맷을 `uint64_t` ID로 관리
- 체력 값을 난독화해 보관하는 `Memory::ProtectedInt`

## 실행 방법

1. [DirectX.sln](DirectX.sln)을 Visual Studio에서 엽니다.
2. 구성은 `Debug | x64` 또는 `Release | x64`를 선택합니다.
3. 빌드 후 실행합니다. 결과 실행 파일은 `bin` 폴더에 생성됩니다.

필요 환경:

- Visual Studio 2022 이상
- MSVC C++20 컴파일러
- Windows 10/11 SDK

별도의 CMake 또는 외부 라이브러리 설치는 필요하지 않습니다. 프로젝트는 Windows SDK의 `d3d11`, `dxgi`, `d3dcompiler`, `d2d1`, `dwrite`, `user32` 라이브러리를 사용합니다.

## 조작

| 키 | 동작 |
| --- | --- |
| `W` | 위로 이동 |
| `A` | 왼쪽으로 이동 |
| `S` | 아래로 이동 |
| `D` | 오른쪽으로 이동 |

## 현재 게임 흐름

1. `main.cpp`에서 렌더러와 `Scene`을 생성합니다.
2. `Scene::Initialize()`가 플레이어와 타이머를 초기화합니다.
3. 처음 2초 뒤 적과 중립 오브젝트가 생성되고, 이후 5초 간격으로 추가 생성됩니다.
4. 적은 플레이어를 향해 이동하며 충돌하면 체력을 하나 감소시킵니다.
5. 초록색 중립 오브젝트에 닿으면 임의의 적 하나가 제거됩니다.
6. 체력이 0이면 `Game Over` 텍스트를 표시합니다.

오브젝트 좌표는 **데스크톱 절대 픽셀 좌표**이며, 사각형의 `x`, `y`는 좌측 상단이 아니라 중심을 가리킵니다.

## 주요 구조

| 구성 요소 | 역할 |
| --- | --- |
| `Renderer` | DirectX 11 사각형 렌더링, Direct2D/DirectWrite 텍스트 렌더링, 창 크기 변경 처리 |
| `Scene` | 플레이어·오브젝트·충돌·스케일·씬 전용 텍스트·타이머 관리 |
| `GameState` | 플레이어 사각형, 생존 여부, 체력 상태 보관 |
| `TimerManager` | `AddOnce`, `AddRepeat`, `AddRepeatAfter` 이벤트 관리 |
| `ControlAI` | 오브젝트의 이동과 충돌 반응 함수 |
| `UI` | 전역 텍스트 요청을 ID로 관리 |
| `ProtectedValue` | 정수 값의 난독화 및 무결성 검증 |

## 텍스트 사용 예시

텍스트와 텍스트 포맷은 자동 번호가 아닌, 사람이 의미를 알 수 있는 상수 ID로 생성합니다.

```cpp
constexpr UI::TextFormatId TEXT_FORMAT_UI = 1;
constexpr UI::TextId TEXT_UI_LIFE = 1;

gRenderer.CreateTextFormat(
    TEXT_FORMAT_UI,
    L"Arial",
    32.0f,
    DWRITE_TEXT_ALIGNMENT_LEADING,
    DWRITE_PARAGRAPH_ALIGNMENT_NEAR
);

UI::CreateText(
    TEXT_UI_LIFE,
    L"",
    TEXT_FORMAT_UI,
    UI::TextAnchor::TopLeft,
    10.0f, 10.0f,
    240.0f, 60.0f
);
```

`TextAnchor`는 `TopLeft`, `TopRight`, `Top`, `Center`, `BottomLeft`, `BottomRight`, `Bottom`, `Left`, `Right`를 지원합니다. 앵커 위치는 렌더링 시 현재 창 크기로 계산되므로 창 크기가 바뀌어도 UI 위치가 다시 맞춰집니다.

## 체력 값 난독화

`GameState::_life`는 평문 `int`가 아니라 `Memory::ProtectedInt`입니다.

```cpp
gameState._life.Set(3);
const int life = gameState._life.Get();
```

저장할 때 값마다 키와 검증값을 갱신합니다. 이는 단순 메모리 값 검색이나 임의 수정의 난이도를 높이는 용도이며, 클라이언트에서 실행되는 값을 완전히 보호하는 보안 수단은 아닙니다.

## 다음 확장 방향

- `TitleScene`, `GameScene`, `GameOverScene`을 분리하는 `SceneManager` 도입
- 이미지/스프라이트 렌더링
- 키 입력 기반 일시정지 및 시작 화면
- 서버 권한 기반 게임 상태 검증
