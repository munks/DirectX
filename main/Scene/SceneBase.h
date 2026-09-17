#pragma once

#include <sal.h>
#include <memory>

class Renderer;

struct SceneData {
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float speed;
};

class SceneBase {
    public:
        virtual ~SceneBase() = default;

        static void SetInitialScene(std::unique_ptr<SceneBase> scene, _In_ Renderer* renderer);
        static void RequestSceneChange(std::unique_ptr<SceneBase> scene);
        static void UpdateCurrentScene();

    protected:
        // 파생 씬은 필요하면 SceneBase::Initialize(renderer)를 먼저 호출한다.
        // 그러면 각 씬이 공통 Renderer 포인터를 안전하게 사용할 수 있다.
        virtual void Initialize(_In_ Renderer* renderer);
        virtual void Update() = 0;

        Renderer* GetRenderer() const { return _renderer; }

    private:
        // Update 도중 현재 씬 자신을 삭제하지 않도록, 전환 요청은 다음 프레임 경계까지 보류한다.
        inline static std::unique_ptr<SceneBase> _currentScene;
        inline static std::unique_ptr<SceneBase> _pendingScene;
        inline static Renderer* _sceneRenderer = nullptr;
        Renderer* _renderer = nullptr;
};
