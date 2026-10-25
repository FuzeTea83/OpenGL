#include "UpdateParticleSystem.h"

#include "ecs/core/Entity.h"

UpdateParticleSystem::UpdateParticleSystem(const Priority priority) : UpdateSystem(priority) {}

void UpdateParticleSystem::update(const Engine &engine, Entity &entity, float deltaTime)
{
    auto &transform = entity.getComponent<TransformComponent>();
    //auto &particle = entity.getComponent<ParticleComponent>();
}
