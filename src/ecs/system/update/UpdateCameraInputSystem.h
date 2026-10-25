#ifndef OPENGL_UPDATE_CAMERA_INPUT_SYSTEM_H
#define OPENGL_UPDATE_CAMERA_INPUT_SYSTEM_H

#include "core/input/Input.h"
#include "ecs/core/System.h"
#include "ecs/component/camera/CameraComponent.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/component/camera/MainCameraTag.h"

class UpdateCameraInputSystem : public UpdateSystem<TransformComponent, CameraComponent, MainCameraTag>
{
public:
    explicit UpdateCameraInputSystem(const Priority priority) : UpdateSystem(priority) {}

    void update(const Engine &engine, Entity &entity, float deltaTime) override;

    void start(const Engine &engine) override { mExecuted = false; }

private:
    static void handleMouseInput(const Engine &engine, CameraComponent &camera);

    static void handleMovementInput(const Engine &engine, Transform &current);

    bool mExecuted = false;
};
#endif //OPENGL_UPDATE_CAMERA_INPUT_SYSTEM_H
