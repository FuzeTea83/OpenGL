#ifndef OPENGL_ANIMATOR_COMPONENT_H
#define OPENGL_ANIMATOR_COMPONENT_H

#include <vector>
#include <string>
#include <variant>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ecs/core/Component.h"
#include "utils/Ease.h"

using AnimValue = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::quat>;

json animValueToJson(const AnimValue &val);
AnimValue animValueFromJson(const json &item);

struct Keyframe
{
    float time = 0.0f;
    AnimValue value = 0.0f;
    Ease ease = Ease::Linear;
};

json keyframeToJson(const Keyframe &kf);
Keyframe keyframeFromJson(const json &j);

struct AnimationTrack
{
    std::string componentName; // Nom ou ID du composant (ex: "TransformComponent", "LightComponent", etc.)
    std::string propertyName;  // Nom de la propriété dans le Codec (ex: "Position", "Rotation", "Scale", "Color", "Intensity")
    std::vector<Keyframe> keyframes;
    bool isRecording = false;

    void sortKeyframes()
    {
        std::ranges::sort(keyframes, [](const Keyframe &a, const Keyframe &b) {
            return a.time < b.time;
        });
    }

    void addOrUpdateKeyframe(float time, const AnimValue &val, Ease ease = Ease::Linear)
    {
        constexpr float EPSILON = 0.001f;
        for (auto &kf : keyframes)
        {
            if (std::abs(kf.time - time) < EPSILON)
            {
                kf.value = val;
                kf.ease = ease;
                return;
            }
        }
        keyframes.push_back(Keyframe{time, val, ease});
        sortKeyframes();
    }

    void removeKeyframe(size_t index)
    {
        if (index < keyframes.size())
        {
            keyframes.erase(keyframes.begin() + index);
        }
    }

    AnimValue evaluate(float time) const;
};

json trackToJson(const AnimationTrack &track);
AnimationTrack trackFromJson(const json &j);

struct Animator : Component
{
    std::string clipName = "Default Clip";
    std::vector<AnimationTrack> tracks;
    float duration = 5.0f;
    float speed = 1.0f;
    bool isPlaying = false;
    bool isLooping = true;
    bool isRecording = false;
    float currentTime = 0.0f;

    Codec::CustomJson tracksCodec;

    Animator() = default;
    Animator(const Animator &other)
        : Component(other),
          clipName(other.clipName),
          tracks(other.tracks),
          duration(other.duration),
          speed(other.speed),
          isPlaying(other.isPlaying),
          isLooping(other.isLooping),
          isRecording(other.isRecording),
          currentTime(other.currentTime)
    {
    }

    Animator &operator=(const Animator &other)
    {
        if (this != &other)
        {
            Component::operator=(other);
            clipName = other.clipName;
            tracks = other.tracks;
            duration = other.duration;
            speed = other.speed;
            isPlaying = other.isPlaying;
            isLooping = other.isLooping;
            isRecording = other.isRecording;
            currentTime = other.currentTime;
        }
        return *this;
    }

    void initializeCodec(const Engine &engine) override;

    json toJson() const;
    void fromJson(const json &j);

    json tracksToJson() const;
    void tracksFromJson(const json &j);

    void play() { isPlaying = true; }
    void pause() { isPlaying = false; }
    void stop()
    {
        isPlaying = false;
        currentTime = 0.0f;
    }
    void setTime(const float t)
    {
        currentTime = glm::clamp(t, 0.0f, duration > 0.0f ? duration : 0.0f);
    }

    AnimationTrack *findTrack(const std::string &compName, const std::string &propName)
    {
        for (auto &track : tracks)
        {
            if (track.componentName == compName && track.propertyName == propName)
            {
                return &track;
            }
        }
        return nullptr;
    }

    AnimationTrack &getOrCreateTrack(const std::string &compName, const std::string &propName)
    {
        if (auto *t = findTrack(compName, propName))
        {
            return *t;
        }
        tracks.push_back(AnimationTrack{.componentName = compName, .propertyName = propName, .keyframes = {}, .isRecording = false});
        return tracks.back();
    }

    void removeTrack(size_t index)
    {
        if (index < tracks.size())
        {
            tracks.erase(tracks.begin() + index);
        }
    }

    Clonable *clone() const override
    {
        return new Animator(*this);
    }
};

#endif //OPENGL_ANIMATOR_COMPONENT_H
