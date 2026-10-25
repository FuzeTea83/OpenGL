#ifndef OPENGL_SYSTEM_H
#define OPENGL_SYSTEM_H

#include <utility>

#include "Signature.h"

class Engine;
class Entity;

using Priority = unsigned int;

class System
{
public:
    explicit System(const Priority priority, Signature signature)
        : mPriority(priority), mSignature(std::move(signature)) {}

    virtual ~System() = default;

    Priority getPriority() const { return mPriority; }

    const Signature &getSignature() const { return mSignature; }

private:
    Priority mPriority;
    Signature mSignature;
};

class IUpdateSystem : public System
{
public:
    explicit IUpdateSystem(const Priority priority, const Signature &signature) : System(priority, signature) {}

    virtual void start(const Engine &engine) {}

    virtual void update(const Engine &engine, Entity &entity, float deltaTime) {}

    virtual void stop(const Engine &engine) {}
};

class IRenderSystem : public System
{
public:
    explicit IRenderSystem(const Priority priority, const Signature &signature) : System(priority, signature) {}

    virtual void start(const Engine &engine) {}

    virtual void render(const Engine &engine, Entity &entity, float alpha) {};

    virtual void stop(const Engine &engine) {}
};

template<typename... Components>
class UpdateSystem : public IUpdateSystem
{
public:
    static inline const Signature signature = Signature::create<Components...>();

    explicit UpdateSystem(const Priority priority) : IUpdateSystem(priority, signature) {}
};

template<typename... Components>
class RenderSystem : public IRenderSystem
{
public:
    static inline const Signature signature = Signature::create<Components...>();

    explicit RenderSystem(const Priority priority) : IRenderSystem(priority, signature) {}
};
#endif //OPENGL_SYSTEM_H
