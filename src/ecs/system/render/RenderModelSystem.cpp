#include "RenderModelSystem.h"

#include "core/Engine.h"
#include "core/renderer/SceneRenderer.h"
#include "ecs/component/ModelAnimator.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/core/Entity.h"

RenderModelSystem::RenderModelSystem(const Priority priority) : RenderSystem(priority) {}

void RenderModelSystem::render(const Engine &engine, Entity &entity, const float alpha)
{
    const auto &transform = entity.getComponent<TransformComponent>();
    const auto &model = entity.getComponent<ModelComponent>();

    if (model.definition.id == INVALID_MESH_ID) { return; }

    MeshManager &meshManager = engine.getMeshManager();
    MaterialManager &materialManager = engine.getMaterialManager();
    auto &renderer = engine.getSceneRenderer();
    const Mesh &mesh = meshManager.getMesh(model.definition.id);

    const bool hasAnimator = entity.hasComponent<ModelAnimator>();
    const auto *animator = hasAnimator ? &entity.getComponent<ModelAnimator>() : nullptr;

    const glm::mat4 entityModel = transform.getModelMatrix(alpha);

    for (const auto &sub: mesh.subMeshes)
    {
        DrawCommand cmd{};
        cmd.outline = model.outline;
        cmd.shaderId = renderer.getShaderId(ShaderType::Main);
        cmd.vaoId = sub.vao;
        cmd.indexCount = sub.indiceCount;
        cmd.vertexCount = sub.vertexCount;

        if (hasAnimator && animator->mInitialized)
        {
            if (sub.skinIndex >= 0 && sub.skinIndex < static_cast<int>(animator->skinJointMatrices.size()))
            {
                cmd.boneMatrices = &animator->skinJointMatrices[sub.skinIndex];
                cmd.model = entityModel;
            }
            else if (sub.nodeIndex >= 0 && sub.nodeIndex < static_cast<int>(animator->nodeGlobalTransforms.size()))
            {
                cmd.model = entityModel * animator->nodeGlobalTransforms[sub.nodeIndex];
            }
            else
            {
                cmd.model = entityModel * sub.nodeTransform;
            }
        }
        else
        {
            cmd.model = entityModel * sub.nodeTransform;
        }

        cmd.material = materialManager.getMaterial(sub.materialId);
        cmd.emissiveColor = model.emissiveColor;
        cmd.emissivePartsOnly = model.emissivePartsOnly;
        cmd.emissiveIntensity = model.emissiveIntensity;
        cmd.emissiveOnly = model.emissiveOnly;
        renderer.addDrawCommand(cmd);
    }
}
