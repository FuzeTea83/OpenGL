#include "RenderLightSystem.h"

#include "core/Engine.h"
#include "core/renderer/SceneRenderer.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/component/light/LightComponent.h"
#include "ecs/component/light/variant/AreaLightVariant.h"
#include "ecs/component/light/variant/DirectionalLightVariant.h"
#include "ecs/component/light/variant/NeonLightVariant.h"
#include "ecs/component/light/variant/PointLightVariant.h"
#include "ecs/component/light/variant/SphereLightVariant.h"
#include "ecs/component/light/variant/SpotLightVariant.h"
#include "ecs/core/ECS.h"
#include "ecs/core/Entity.h"

RenderLightSystem::RenderLightSystem(const Priority priority) : RenderSystem(priority) {}

void RenderLightSystem::start(const Engine &engine) { lightCount = 0; }


void RenderLightSystem::render(const Engine &engine, Entity &entity, float alpha)
{
    if (lightCount >= MAX_LIGHTS) { return; }

    const auto &transform = entity.getComponent<TransformComponent>();
    auto &light = entity.getComponent<LightComponent>();

    int type;
    glm::vec3 direction{0};
    glm::vec3 attenuation{0};
    glm::vec2 spotParams{0};
    float radius{0};

    if (const auto directionalLightVariant = dynamic_cast<DirectionalLightVariant *>(light.variant.get()))
    {
        type = 0;
        direction = directionalLightVariant->direction;
    }
    else if (const auto pointLightVariant = dynamic_cast<PointLightVariant *>(light.variant.get()))
    {
        type = 1;
        attenuation = pointLightVariant->attenuation;
    }
    else if (const auto spotLightVariant = dynamic_cast<SpotLightVariant *>(light.variant.get()))
    {
        type = 2;
        direction = spotLightVariant->followRotation
                        ? transform.current.rotation * spotLightVariant->direction
                        : spotLightVariant->direction;
        attenuation = spotLightVariant->attenuation;
        spotParams = glm::vec2(spotLightVariant->innerCutoff, spotLightVariant->outerCutoff);
    }
    else if (const auto neonLightVariant = dynamic_cast<NeonLightVariant *>(light.variant.get()))
    {
        type = 3;
        direction = neonLightVariant->direction;
        attenuation = neonLightVariant->attenuation;
        radius = neonLightVariant->radius;
    }
    else if (const auto sphereLightVariant = dynamic_cast<SphereLightVariant *>(light.variant.get()))
    {
        type = 4;
        attenuation = sphereLightVariant->attenuation;
        radius = sphereLightVariant->radius;
    }
    else if (const auto areaLightVariant = dynamic_cast<AreaLightVariant *>(light.variant.get()))
    {
        type = 5;
        direction = areaLightVariant->direction;
        attenuation = areaLightVariant->attenuation;
        spotParams = glm::vec2(areaLightVariant->width, areaLightVariant->height);
    }
    else
    {
        return;
    }

    auto &uniform = lights[lightCount++];

    uniform.position = glm::vec4(transform.current.position + light.offset, type);
    uniform.color = glm::vec4(light.color, light.intensity);
    uniform.direction = glm::vec4(direction, light.ambient);
    uniform.attenuation = glm::vec4(attenuation, light.diffuse);
    uniform.spotParams = glm::vec4(spotParams.x, spotParams.y, radius, light.specular);
}

void RenderLightSystem::stop(const Engine &engine) { engine.getSceneRenderer().setLightUniforms(lights, lightCount); }
