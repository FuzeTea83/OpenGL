#include "RenderMainCameraSystem.h"

#include "core/Engine.h"
#include "ecs/component/camera/variant/FpsCameraVariant.h"
#include "ecs/component/camera/variant/FreeCameraVariant.h"
#include "ecs/component/camera/variant/OrbitalCameraVariant.h"
#include "ecs/core/Entity.h"

RenderMainCameraSystem::RenderMainCameraSystem(const Priority priority) : RenderSystem(priority) {}

void RenderMainCameraSystem::render(const Engine &engine, Entity &entity, const float alpha)
{
    if (mGlobalUniformsSet) { return; }
    const auto &transform = entity.getComponent<TransformComponent>();
    const auto &camera = entity.getComponent<CameraComponent>();

    if (std::isnan(camera.aspectRatio)) { return; }
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.fov),
        camera.aspectRatio,
        camera.nearPlane,
        camera.farPlane);

    glm::quat rotation;
    glm::vec3 camPos;

    if (const auto *fpsCameraVariant = dynamic_cast<FpsCameraVariant *>(camera.variant.get()))
    {
        rotation = glm::quat(glm::vec3(glm::radians(camera.pitch), glm::radians(camera.yaw), 0.0f));
        camPos = glm::mix(transform.previous.position, transform.current.position, alpha) + fpsCameraVariant->offset;
    }
    else if (const auto *orbitalCameraVariant = dynamic_cast<OrbitalCameraVariant *>(camera.variant.get()))
    {
        rotation = glm::quat(glm::vec3(glm::radians(camera.pitch), glm::radians(camera.yaw), 0.0f));
        camPos = glm::mix(transform.previous.position, transform.current.position, alpha)
                 - rotation * glm::vec3(0.0f, 0.0f, -orbitalCameraVariant->distance)
                 + orbitalCameraVariant->offset;
    }
    else if (const auto *freeCameraVariant = dynamic_cast<FreeCameraVariant *>(camera.variant.get()))
    {
        rotation = freeCameraVariant->transform.rotation;
        camPos = freeCameraVariant->transform.position;
    }
    else { return; }

    const glm::mat4 view = glm::mat4_cast(glm::conjugate(rotation));
    const glm::mat4 viewTranslated = view * glm::translate(glm::mat4(1.0f), -camPos);

    engine.getSceneRenderer().setGlobalUniforms(projection, view, viewTranslated, camPos);
    mGlobalUniformsSet = true;
}
