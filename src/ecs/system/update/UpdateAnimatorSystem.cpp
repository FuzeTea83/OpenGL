#include "UpdateAnimatorSystem.h"
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include "ecs/component/Animator.h"
#include "ecs/core/ECS.h"
#include "ecs/core/Entity.h"

namespace {

void applyValueToCodecProperty(Codec::Type type, const Codec::property &targetProp, const AnimValue &val)
{
    std::visit([type, &val](auto *ptr) {
        if (ptr == nullptr) { return; }

        using TargetPtrType = std::decay_t<decltype(ptr)>;

        if constexpr (std::is_same_v<TargetPtrType, bool *>)
        {
            if (std::holds_alternative<bool>(val)) { *ptr = std::get<bool>(val); }
            else if (std::holds_alternative<float>(val)) { *ptr = std::get<float>(val) > 0.5f; }
            else if (std::holds_alternative<int>(val)) { *ptr = std::get<int>(val) != 0; }
        }
        else if constexpr (std::is_same_v<TargetPtrType, int *>)
        {
            if (std::holds_alternative<int>(val)) { *ptr = std::get<int>(val); }
            else if (std::holds_alternative<float>(val)) { *ptr = static_cast<int>(std::round(std::get<float>(val))); }
            else if (std::holds_alternative<bool>(val)) { *ptr = std::get<bool>(val) ? 1 : 0; }
        }
        else if constexpr (std::is_same_v<TargetPtrType, float *>)
        {
            if (std::holds_alternative<float>(val)) { *ptr = std::get<float>(val); }
            else if (std::holds_alternative<int>(val)) { *ptr = static_cast<float>(std::get<int>(val)); }
            else if (std::holds_alternative<bool>(val)) { *ptr = std::get<bool>(val) ? 1.0f : 0.0f; }
        }
        else if constexpr (std::is_same_v<TargetPtrType, glm::vec2 *>)
        {
            if (std::holds_alternative<glm::vec2>(val)) { *ptr = std::get<glm::vec2>(val); }
            else if (std::holds_alternative<float>(val)) { *ptr = glm::vec2(std::get<float>(val)); }
        }
        else if constexpr (std::is_same_v<TargetPtrType, glm::vec3 *>)
        {
            if (std::holds_alternative<glm::vec3>(val)) { *ptr = std::get<glm::vec3>(val); }
            else if (std::holds_alternative<glm::vec4>(val)) { *ptr = glm::vec3(std::get<glm::vec4>(val)); }
            else if (std::holds_alternative<float>(val)) { *ptr = glm::vec3(std::get<float>(val)); }
        }
        else if constexpr (std::is_same_v<TargetPtrType, glm::vec4 *>)
        {
            if (std::holds_alternative<glm::vec4>(val)) { *ptr = std::get<glm::vec4>(val); }
            else if (std::holds_alternative<glm::vec3>(val)) { *ptr = glm::vec4(std::get<glm::vec3>(val), 1.0f); }
            else if (std::holds_alternative<float>(val)) { *ptr = glm::vec4(std::get<float>(val)); }
        }
        else if constexpr (std::is_same_v<TargetPtrType, glm::quat *>)
        {
            if (std::holds_alternative<glm::quat>(val)) { *ptr = std::get<glm::quat>(val); }
            else if (std::holds_alternative<glm::vec3>(val))
            {
                // Interpréter comme des angles d'Euler en radians ou degrés
                const glm::vec3 euler = std::get<glm::vec3>(val);
                *ptr = glm::quat(glm::radians(euler));
            }
        }
    }, targetProp);
}

void applyTrackToEntity(const Entity &entity, const AnimationTrack &track, const AnimValue &val)
{
    for (Component *comp : entity.getComponents())
    {
        if (!comp) continue;

        // Si un nom de composant spécifique est requis, on vérifie s'il correspond (ou vide pour matcher sur le nom de propriété)
        auto &props = comp->codec.properties;
        auto it = props.find(track.propertyName);
        if (it != props.end())
        {
            applyValueToCodecProperty(it->second.first, it->second.second, val);
        }

        // Vérifier les sous-codecs récursivement si nécessaire
        for (const auto &[name, pair] : props)
        {
            if (pair.first == Codec::Type::CODEC)
            {
                if (auto *subCodec = std::get<Codec *>(pair.second))
                {
                    auto subIt = subCodec->properties.find(track.propertyName);
                    if (subIt != subCodec->properties.end())
                    {
                        applyValueToCodecProperty(subIt->second.first, subIt->second.second, val);
                    }
                }
            }
        }
    }
}

} // namespace

void UpdateAnimatorSystem::update(const Engine &engine, Entity &entity, const float deltaTime)
{
    auto &animator = entity.getComponent<Animator>();
    if (!animator.isPlaying || animator.tracks.empty()) { return; }

    animator.currentTime += deltaTime * animator.speed;

    if (animator.duration > 0.0f && animator.currentTime > animator.duration)
    {
        if (animator.isLooping)
        {
            animator.currentTime = std::fmod(animator.currentTime, animator.duration);
        }
        else
        {
            animator.currentTime = animator.duration;
            animator.isPlaying = false;
        }
    }

    for (const auto &track : animator.tracks)
    {
        if (track.keyframes.empty()) continue;

        AnimValue val = track.evaluate(animator.currentTime);
        applyTrackToEntity(entity, track, val);
    }
}
