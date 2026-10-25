#ifndef OPENGL_RENDER_LIGHT_SYSTEM_H
#define OPENGL_RENDER_LIGHT_SYSTEM_H

#include "core/renderer/SceneRenderer.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/component/light/LightComponent.h"
#include "ecs/core/System.h"

class RenderLightSystem : public RenderSystem<TransformComponent, LightComponent>
{
public:
    explicit RenderLightSystem(Priority priority);

    void start(const Engine &engine) override;

    void render(const Engine &engine, Entity &entity, float alpha) override;

    void stop(const Engine &engine) override;

private:
    std::array<LightUniforms, MAX_LIGHTS> lights{};
    int lightCount{};
};

#endif //OPENGL_RENDER_LIGHT_SYSTEM_H