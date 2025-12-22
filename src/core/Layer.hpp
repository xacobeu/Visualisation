#pragma once

class Application;
class Renderer;

class Layer {
public:
    virtual ~Layer() = default;

    virtual void onAttach() {}
    virtual void onDetach() {}

    virtual void onUpdate(float deltaTime) {}
    virtual void onRender(Renderer& renderer) {}

protected:
    Application* app = nullptr;

    friend class Application;
};
