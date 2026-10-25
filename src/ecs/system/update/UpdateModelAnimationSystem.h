#ifndef OPENGL_UPDATE_ANIMATION_SYSTEM_H
#define OPENGL_UPDATE_ANIMATION_SYSTEM_H

#include "ecs/component/ModelAnimator.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/core/System.h"

class UpdateModelAnimationSystem : public UpdateSystem<ModelAnimator, ModelComponent>
{
public:
    explicit UpdateModelAnimationSystem(Priority priority);

    void update(const Engine &engine, Entity &entity, float deltaTime) override;
};

#endif //OPENGL_UPDATE_ANIMATION_SYSTEM_H