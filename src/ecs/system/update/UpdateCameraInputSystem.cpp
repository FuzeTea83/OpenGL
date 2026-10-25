#include "UpdateCameraInputSystem.h"

#include <iostream>

#include "core/Engine.h"
#include "core/input/Input.h"
#include "ecs/component/camera/CameraComponent.h"
#include "ecs/component/camera/variant/FpsCameraVariant.h"
#include "ecs/component/camera/variant/FreeCameraVariant.h"
#include "ecs/component/camera/variant/OrbitalCameraVariant.h"
#include "ecs/core/Entity.h"

#define MOVE_SPEED 1.0f

void UpdateCameraInputSystem::update(const Engine &engine, Entity &entity, float deltaTime)
{
    if (mExecuted) { return; }
    auto &transform = entity.getComponent<TransformComponent>();
    auto &camera = entity.getComponent<CameraComponent>();

    if (dynamic_cast<FpsCameraVariant *>(camera.variant.get()))
    {
        handleMouseInput(engine, camera);
        handleMovementInput(engine, transform.current);
        transform.current.rotation = glm::quat(glm::vec3(glm::radians(camera.pitch), glm::radians(camera.yaw), 0.0f));
    }
    if (dynamic_cast<OrbitalCameraVariant *>(camera.variant.get()))
    {
        handleMouseInput(engine, camera);
        handleMovementInput(engine, transform.current);
    }
    if (auto *freeCameraVariant = dynamic_cast<FreeCameraVariant *>(camera.variant.get()))
    {
        handleMouseInput(engine, camera);
        handleMovementInput(engine, freeCameraVariant->transform);
        freeCameraVariant->transform.rotation = glm::quat(
            glm::vec3(glm::radians(camera.pitch), glm::radians(camera.yaw), 0.0f));
    }
    mExecuted = true;
}



void UpdateCameraInputSystem::handleMouseInput(const Engine &engine, CameraComponent &camera)
{
    const ECS &ecs = engine.getECS();
    const Input &input = engine.getInput();
    if (!ecs.canInterpretInputs() || !input.isMouseLocked()) { return; }
    const float deltaYaw = input.getDeltaX() * input.getSensitivity();
    const float deltaPitch = input.getDeltaY() * input.getSensitivity();

    camera.yaw = fmod(camera.yaw + deltaYaw, 360.0f);
    camera.pitch = glm::clamp(camera.pitch + deltaPitch, -89.0f, 89.0f);
}

void UpdateCameraInputSystem::handleMovementInput(const Engine &engine, Transform &current)
{
    const ECS &ecs = engine.getECS();
    if (!ecs.canInterpretInputs()) { return; }
    const Input &input = engine.getInput();

    if (input.isPressed(Input::Action::MoveUp)) { current.position += MOVE_SPEED * glm::vec3(0.0f, 1.0f, 0.0f); }
    if (input.isPressed(Input::Action::MoveDown)) { current.position += MOVE_SPEED * -glm::vec3(0.0f, 1.0f, 0.0f); }
    if (input.isPressed(Input::Action::MoveForward)) { current.position += MOVE_SPEED * current.getForward(); }
    if (input.isPressed(Input::Action::MoveBackward)) { current.position += MOVE_SPEED * -current.getForward(); }
    if (input.isPressed(Input::Action::MoveRight)) { current.position += MOVE_SPEED * current.getRight(); }
    if (input.isPressed(Input::Action::MoveLeft)) { current.position += MOVE_SPEED * -current.getRight(); }
}
