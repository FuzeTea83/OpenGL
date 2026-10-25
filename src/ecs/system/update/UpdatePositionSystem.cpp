#include "UpdatePositionSystem.h"

#include "ecs/component/TransformComponent.h"
#include "ecs/core/ECS.h"
#include "ecs/core/Entity.h"


UpdatePositionSystem::UpdatePositionSystem(const Priority priority) : UpdateSystem(priority) {}

void UpdatePositionSystem::update(const Engine &engine, Entity &entity, float deltaTime)
{
    auto &transform = entity.getComponent<TransformComponent>();
    transform.previous = transform.current;
}
