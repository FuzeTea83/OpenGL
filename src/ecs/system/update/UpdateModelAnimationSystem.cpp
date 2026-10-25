#include "UpdateModelAnimationSystem.h"

#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ecs/component/ModelAnimator.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/core/ECS.h"

static std::size_t findInterval(const std::vector<float> &timestamps, const float t)
{
    if (timestamps.empty()) { return 0; }
    std::size_t low = 0;
    std::size_t high = timestamps.size() - 1;
    while (low + 1 < high)
    {
        const std::size_t mid = (low + high) / 2;
        if (timestamps[mid] <= t) { low = mid; }
        else{ high = mid;}
    }
    return low;
}

static glm::vec3 sampleVec3(const AnimChannel &ch, const float t)
{
    if (ch.timestamps.empty() || ch.vec3Values.empty()) { return glm::vec3(0.0f); }
    if (t <= ch.timestamps.front() || ch.timestamps.size() == 1) {return ch.vec3Values.front();}
    if (t >= ch.timestamps.back()) {return ch.vec3Values.back();}

    const std::size_t i = findInterval(ch.timestamps, t);
    const float t0 = ch.timestamps[i];
    const float t1 = ch.timestamps[i + 1];
    const float factor = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;

    if (ch.interpolation == AnimInterpolation::Step) { return ch.vec3Values[i]; }
    return glm::mix(ch.vec3Values[i], ch.vec3Values[i + 1], factor);
}

static glm::quat sampleQuat(const AnimChannel &ch, const float t)
{
    if (ch.timestamps.empty() || ch.quatValues.empty()) { return {1.0f, 0.0f, 0.0f, 0.0f}; }
    if (t <= ch.timestamps.front() || ch.timestamps.size() == 1) { return ch.quatValues.front(); }
    if (t >= ch.timestamps.back()) { return ch.quatValues.back(); }

    const std::size_t i = findInterval(ch.timestamps, t);
    const float t0 = ch.timestamps[i];
    const float t1 = ch.timestamps[i + 1];
    const float factor = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;

    if (ch.interpolation == AnimInterpolation::Step) { return ch.quatValues[i]; }
    return glm::normalize(glm::slerp(ch.quatValues[i], ch.quatValues[i + 1], factor));
}

static void computeNodeTransform(const int nodeIdx, const glm::mat4 &parentMat,
                                 const std::vector<Node> &nodes,
                                 std::vector<glm::mat4> &outGlobals)
{
    const auto &n = nodes[nodeIdx];
    glm::mat4 localMat;
    if (n.hasMatrix) { localMat = n.matrix; }
    else
    {
        localMat = glm::translate(glm::mat4(1.0f), n.translation)
                   * glm::mat4_cast(n.rotation)
                   * glm::scale(glm::mat4(1.0f), n.scale);
    }
    const glm::mat4 globalMat = parentMat * localMat;
    outGlobals[nodeIdx] = globalMat;

    for (const int childIdx: n.children)
    {
        if (childIdx >= 0 && childIdx < static_cast<int>(nodes.size()))
        {
            computeNodeTransform(childIdx, globalMat, nodes, outGlobals);
        }
    }
}

UpdateModelAnimationSystem::UpdateModelAnimationSystem(const Priority priority) : UpdateSystem(priority) {}

void UpdateModelAnimationSystem::update(const Engine &engine, Entity &entity, const float deltaTime)
{
    auto &animator = entity.getComponent<ModelAnimator>();
    const auto &model = entity.getComponent<ModelComponent>();
    if (model.definition.id == INVALID_MESH_ID) { return; }
    const Mesh &mesh = engine.getMeshManager().getMesh(model.definition.id);

    if (mesh.nodes.empty()) return;

    if (!animator.mInitialized || animator.runtimeNodes.size() != mesh.nodes.size())
    {
        animator.animationNames.clear();
        for (const auto &animation: mesh.animations) { animator.animationNames.push_back(animation.name); }
        animator.codec.markDirty();
        animator.runtimeNodes = mesh.nodes;
        animator.nodeGlobalTransforms.resize(mesh.nodes.size(), glm::mat4(1.0f));
        animator.skinJointMatrices.resize(mesh.skins.size());
        for (std::size_t s = 0; s < mesh.skins.size(); ++s)
        {
            animator.skinJointMatrices[s].resize(mesh.skins[s].joints.size(), glm::mat4(1.0f));
        }
        animator.mInitialized = true;
    }

    if (mesh.animations.empty())
    {
        for (std::size_t i = 0; i < animator.runtimeNodes.size(); ++i)
        {
            if (animator.runtimeNodes[i].parent == -1)
            {
                computeNodeTransform(static_cast<int>(i), glm::mat4(1.0f), animator.runtimeNodes,
                                     animator.nodeGlobalTransforms);
            }
        }
        if (animator.skinJointMatrices.size() != mesh.skins.size())
        {
            animator.skinJointMatrices.resize(mesh.skins.size());
        }
        for (std::size_t s = 0; s < mesh.skins.size(); ++s)
        {
            const auto &skin = mesh.skins[s];
            if (animator.skinJointMatrices[s].size() != skin.joints.size())
            {
                animator.skinJointMatrices[s].resize(skin.joints.size(), glm::mat4(1.0f));
            }
            for (std::size_t j = 0; j < skin.joints.size(); ++j)
            {
                const auto &joint = skin.joints[j];
                if (joint.node >= 0 && joint.node < static_cast<int>(animator.nodeGlobalTransforms.size()))
                {
                    animator.skinJointMatrices[s][j] =
                            animator.nodeGlobalTransforms[joint.node] * joint.inverseBindMatrix;
                }
                else { animator.skinJointMatrices[s][j] = glm::mat4(1.0f); }
            }
        }
        return;
    }

    if (!animator.animation.empty())
    {
        for (std::size_t i = 0; i < mesh.animations.size(); ++i)
        {
            if (mesh.animations[i].name == animator.animation)
            {
                animator.mCurrentAnimationIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (animator.mCurrentAnimationIndex < 0 || animator.mCurrentAnimationIndex >= static_cast<int>(mesh.animations.
            size())) { animator.mCurrentAnimationIndex = 0; }

    const auto &clip = mesh.animations[animator.mCurrentAnimationIndex];

    if (animator.mIsPlaying)
    {
        animator.mCurrentTime += deltaTime * animator.mSpeed;
        if (clip.duration > 0.0f)
        {
            if (animator.mIsLooping)
            {
                animator.mCurrentTime = std::fmod(animator.mCurrentTime, clip.duration);
                if (animator.mCurrentTime < 0.0f) animator.mCurrentTime += clip.duration;
            }
            else if (animator.mCurrentTime >= clip.duration)
            {
                animator.mCurrentTime = clip.duration;
                animator.mIsPlaying = false;
            }
        }
    }

    // Reset runtime nodes to mesh rest pose
    animator.runtimeNodes = mesh.nodes;

    // Apply animation channels
    for (const auto &channel: clip.channels)
    {
        if (channel.targetNode < 0 || channel.targetNode >= static_cast<int>(animator.runtimeNodes.size())) continue;

        auto &targetNode = animator.runtimeNodes[channel.targetNode];
        if (targetNode.hasMatrix)
        {
            targetNode.translation = glm::vec3(targetNode.matrix[3]);
            targetNode.hasMatrix = false;
        }

        switch (channel.path)
        {
            case AnimTargetPath::Translation: targetNode.translation = sampleVec3(channel, animator.mCurrentTime);
                break;
            case AnimTargetPath::Rotation: targetNode.rotation = sampleQuat(channel, animator.mCurrentTime);
                break;
            case AnimTargetPath::Scale: targetNode.scale = sampleVec3(channel, animator.mCurrentTime);
                break;
        }
    }

    // Compute global transforms
    for (std::size_t i = 0; i < animator.runtimeNodes.size(); ++i)
    {
        if (animator.runtimeNodes[i].parent == -1)
        {
            computeNodeTransform(static_cast<int>(i), glm::mat4(1.0f), animator.runtimeNodes,
                                 animator.nodeGlobalTransforms);
        }
    }

    // Compute skin matrices if any
    if (animator.skinJointMatrices.size() != mesh.skins.size())
    {
        animator.skinJointMatrices.resize(mesh.skins.size());
    }
    for (std::size_t s = 0; s < mesh.skins.size(); ++s)
    {
        const auto &skin = mesh.skins[s];
        if (animator.skinJointMatrices[s].size() != skin.joints.size())
        {
            animator.skinJointMatrices[s].resize(skin.joints.size(), glm::mat4(1.0f));
        }
        for (std::size_t j = 0; j < skin.joints.size(); ++j)
        {
            const auto &joint = skin.joints[j];
            if (joint.node >= 0 && joint.node < static_cast<int>(animator.nodeGlobalTransforms.size()))
            {
                animator.skinJointMatrices[s][j] = animator.nodeGlobalTransforms[joint.node] * joint.inverseBindMatrix;
            }
            else { animator.skinJointMatrices[s][j] = glm::mat4(1.0f); }
        }
    }
}
