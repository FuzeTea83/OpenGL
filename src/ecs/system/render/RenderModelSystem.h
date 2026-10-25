#ifndef OPENGL_RENDER_MODEL_SYSTEM_H
#define OPENGL_RENDER_MODEL_SYSTEM_H

#include "ecs/core/System.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/component/TransformComponent.h"

class RenderModelSystem : public RenderSystem<TransformComponent, ModelComponent>
{
public:
    explicit RenderModelSystem(Priority priority);

    void render(const Engine &engine, Entity &, float alpha) override;
};

#endif //OPENGL_RENDER_MODEL_SYSTEM_H
