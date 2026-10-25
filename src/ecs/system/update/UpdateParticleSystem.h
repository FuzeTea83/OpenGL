#ifndef OPENGL_UPDATE_PARTICLE_SYSTEM_H
#define OPENGL_UPDATE_PARTICLE_SYSTEM_H

#include "ecs/component/ParticleComponent.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/core/System.h"


class UpdateParticleSystem : UpdateSystem<TransformComponent, ParticleComponent>
{
    explicit UpdateParticleSystem(Priority priority);

    void update(const Engine &engine, Entity &entity, float deltaTime) override;
};


#endif //OPENGL_UPDATE_PARTICLE_SYSTEM_H
