#pragma once
#include <type_traits>

class Application;
class Renderer;

class Layer {
public:
    virtual ~Layer() = default;

    virtual void onAttach() {}
    virtual void onDetach() {}

    virtual void onUpdate(float) {}
    virtual void onRender(Renderer&) {}

protected:
    Application* app = nullptr;

    friend class Application;
};

struct Unique {};

template<typename T>
concept UniqueLayer = std::is_base_of_v<Unique, T>;
