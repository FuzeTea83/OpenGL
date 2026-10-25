#ifndef OPENGL_UPDATE_POSITION_SYSTEM_H
#define OPENGL_UPDATE_POSITION_SYSTEM_H

#include "ecs/component/TransformComponent.h"
#include "ecs/core/System.h"

class UpdatePositionSystem : public UpdateSystem<TransformComponent>
{
public:
    explicit UpdatePositionSystem(Priority priority);

    void update(const Engine &engine, Entity &entity, float deltaTime) override;
};
#endif //OPENGL_UPDATE_POSITION_SYSTEM_H