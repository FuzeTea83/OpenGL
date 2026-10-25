#ifndef OPENGL_UPDATE_ANIMATOR_SYSTEM_H
#define OPENGL_UPDATE_ANIMATOR_SYSTEM_H

#include "ecs/component/Animator.h"
#include "ecs/core/System.h"

class UpdateAnimatorSystem : public UpdateSystem<Animator>
{
public:
    explicit UpdateAnimatorSystem(const Priority priority) : UpdateSystem(priority) {}

    void update(const Engine &engine, Entity &entity, float deltaTime) override;
};
#endif // OPENGL_UPDATE_ANIMATOR_SYSTEM_H