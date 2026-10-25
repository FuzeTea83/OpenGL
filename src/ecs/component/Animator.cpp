#include "Animator.h"
#include <cmath>

namespace {

AnimValue interpolateValues(const AnimValue &v0, const AnimValue &v1, float t)
{
    if (v0.index() != v1.index())
    {
        return v0;
    }

    return std::visit([&]<typename T0>(const T0 &a) -> AnimValue {
        using T = std::decay_t<T0>;
        const auto &b = std::get<T>(v1);

        if constexpr (std::is_same_v<T, bool>)
        {
            return t >= 1.0f ? b : a;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return static_cast<int>(std::round(static_cast<float>(a) + t * static_cast<float>(b - a)));
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return a + t * (b - a);
        }
        else if constexpr (std::is_same_v<T, glm::vec2>)
        {
            return glm::mix(a, b, t);
        }
        else if constexpr (std::is_same_v<T, glm::vec3>)
        {
            return glm::mix(a, b, t);
        }
        else if constexpr (std::is_same_v<T, glm::vec4>)
        {
            return glm::mix(a, b, t);
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            return glm::normalize(glm::slerp(a, b, t));
        }
        else
        {
            return a;
        }
    }, v0);
}

template <typename T>
T hermiteSpline(const T &p0, const T &p1, const T &m0, const T &m1, float u)
{
    const float u2 = u * u;
    const float u3 = u2 * u;
    float h00 = 2.0f * u3 - 3.0f * u2 + 1.0f;
    float h10 = u3 - 2.0f * u2 + u;
    float h01 = -2.0f * u3 + 3.0f * u2;
    float h11 = u3 - u2;
    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

AnimValue interpolateHermite(const AnimValue &v0, const AnimValue &v1,
                             const AnimValue &m0, const AnimValue &m1,
                             float u)
{
    if (v0.index() != v1.index())
    {
        return v0;
    }

    return std::visit([&]<typename T0>(const T0 &p0) -> AnimValue {
        using T = std::decay_t<T0>;
        const auto &p1 = std::get<T>(v1);

        if constexpr (std::is_same_v<T, bool>)
        {
            return u >= 0.5f ? p1 : p0;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            float m0f = static_cast<float>(std::get<int>(m0));
            float m1f = static_cast<float>(std::get<int>(m1));
            float res = hermiteSpline(static_cast<float>(p0), static_cast<float>(p1), m0f, m1f, u);
            return static_cast<int>(std::round(res));
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return hermiteSpline(p0, p1, std::get<float>(m0), std::get<float>(m1), u);
        }
        else if constexpr (std::is_same_v<T, glm::vec2>)
        {
            return hermiteSpline(p0, p1, std::get<glm::vec2>(m0), std::get<glm::vec2>(m1), u);
        }
        else if constexpr (std::is_same_v<T, glm::vec3>)
        {
            return hermiteSpline(p0, p1, std::get<glm::vec3>(m0), std::get<glm::vec3>(m1), u);
        }
        else if constexpr (std::is_same_v<T, glm::vec4>)
        {
            return hermiteSpline(p0, p1, std::get<glm::vec4>(m0), std::get<glm::vec4>(m1), u);
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            glm::quat q0 = p0;
            glm::quat q1 = p1;
            auto tangent0 = std::get<glm::quat>(m0);
            auto tangent1 = std::get<glm::quat>(m1);

            glm::vec4 v0_vec(q0.x, q0.y, q0.z, q0.w);
            glm::vec4 v1_vec(q1.x, q1.y, q1.z, q1.w);
            glm::vec4 m0_vec(tangent0.x, tangent0.y, tangent0.z, tangent0.w);
            glm::vec4 m1_vec(tangent1.x, tangent1.y, tangent1.z, tangent1.w);

            glm::vec4 res_vec = hermiteSpline(v0_vec, v1_vec, m0_vec, m1_vec, u);
            glm::quat res_quat(res_vec.w, res_vec.x, res_vec.y, res_vec.z);
            if (glm::length(res_quat) > 0.0001f)
            {
                return glm::normalize(res_quat);
            }
            return p0;
        }
        else
        {
            return p0;
        }
    }, v0);
}

AnimValue computeTangentDifference(const AnimValue &va, const AnimValue &vb, float scale)
{
    if (va.index() != vb.index())
    {
        return va;
    }

    return std::visit([&]<typename T0>(const T0 &a) -> AnimValue {
        using T = std::decay_t<T0>;
        const auto &b = std::get<T>(vb);

        if constexpr (std::is_same_v<T, bool>)
        {
            return false;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return static_cast<int>(std::round(static_cast<float>(b - a) * scale));
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return (b - a) * scale;
        }
        else if constexpr (std::is_same_v<T, glm::vec2>)
        {
            return (b - a) * scale;
        }
        else if constexpr (std::is_same_v<T, glm::vec3>)
        {
            return (b - a) * scale;
        }
        else if constexpr (std::is_same_v<T, glm::vec4>)
        {
            return (b - a) * scale;
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            glm::quat q_a = a;
            glm::quat q_b = b;
            if (glm::dot(q_a, q_b) < 0.0f)
            {
                q_b = -q_b;
            }
            const glm::vec4 va_vec(q_a.x, q_a.y, q_a.z, q_a.w);
            const glm::vec4 vb_vec(q_b.x, q_b.y, q_b.z, q_b.w);
            const glm::vec4 diff = (vb_vec - va_vec) * scale;
            return glm::quat(diff.w, diff.x, diff.y, diff.z);
        }
        else
        {
            return a;
        }
    }, va);
}

AnimValue createZeroTangent(const AnimValue &val)
{
    return std::visit([]<typename T0>(const T0 &v) -> AnimValue {
        using T = std::decay_t<T0>;
        if constexpr (std::is_same_v<T, bool>) return false;
        else if constexpr (std::is_same_v<T, int>) return 0;
        else if constexpr (std::is_same_v<T, float>) return 0.0f;
        else if constexpr (std::is_same_v<T, glm::vec2>) return glm::vec2(0.0f);
        else if constexpr (std::is_same_v<T, glm::vec3>) return glm::vec3(0.0f);
        else if constexpr (std::is_same_v<T, glm::vec4>) return glm::vec4(0.0f);
        else if constexpr (std::is_same_v<T, glm::quat>) return glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        else return v;
    }, val);
}

} // namespace

json animValueToJson(const AnimValue &val)
{
    json item;
    std::visit([&item]<typename T0>(T0 &&v) {
        using T = std::decay_t<T0>;
        if constexpr (std::is_same_v<T, bool>)
        {
            item["type"] = "bool";
            item["value"] = v;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            item["type"] = "int";
            item["value"] = v;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            item["type"] = "float";
            item["value"] = v;
        }
        else if constexpr (std::is_same_v<T, glm::vec2>)
        {
            item["type"] = "vec2";
            item["value"] = {v.x, v.y};
        }
        else if constexpr (std::is_same_v<T, glm::vec3>)
        {
            item["type"] = "vec3";
            item["value"] = {v.x, v.y, v.z};
        }
        else if constexpr (std::is_same_v<T, glm::vec4>)
        {
            item["type"] = "vec4";
            item["value"] = {v.x, v.y, v.z, v.w};
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            item["type"] = "quat";
            item["value"] = {v.x, v.y, v.z, v.w};
        }
    }, val);
    return item;
}

AnimValue animValueFromJson(const json &item)
{
    if (item.contains("type") && item["type"].is_string() && item.contains("value"))
    {
        const auto type = item["type"].get<std::string>();
        const auto &val = item["value"];
        if (type == "bool" && val.is_boolean()) return val.get<bool>();
        if (type == "int" && val.is_number_integer()) return val.get<int>();
        if (type == "float" && val.is_number()) return val.get<float>();
        if (type == "vec2" && val.is_array() && val.size() >= 2)
            return glm::vec2(val[0].get<float>(), val[1].get<float>());
        if (type == "vec3" && val.is_array() && val.size() >= 3)
            return glm::vec3(val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
        if (type == "vec4" && val.is_array() && val.size() >= 4)
            return glm::vec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>());
        if (type == "quat" && val.is_array() && val.size() >= 4)
            return glm::quat(val[3].get<float>(), val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
    }

    if (item.contains("value"))
    {
        const auto &val = item["value"];
        if (val.is_boolean()) return val.get<bool>();
        if (val.is_number_integer()) return val.get<int>();
        if (val.is_number_float()) return val.get<float>();
        if (val.is_array())
        {
            if (val.size() == 2) return glm::vec2(val[0].get<float>(), val[1].get<float>());
            if (val.size() == 3) return glm::vec3(val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
            if (val.size() == 4) return glm::vec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>());
        }
    }

    if (item.is_boolean()) return item.get<bool>();
    if (item.is_number_integer()) return item.get<int>();
    if (item.is_number_float()) return item.get<float>();
    if (item.is_array())
    {
        if (item.size() == 2) return glm::vec2(item[0].get<float>(), item[1].get<float>());
        if (item.size() == 3) return glm::vec3(item[0].get<float>(), item[1].get<float>(), item[2].get<float>());
        if (item.size() == 4) return glm::vec4(item[0].get<float>(), item[1].get<float>(), item[2].get<float>(), item[3].get<float>());
    }

    return 0.0f;
}

json keyframeToJson(const Keyframe &kf)
{
    json j;
    j["time"] = kf.time;
    j["ease"] = easeToString(kf.ease);
    json val = animValueToJson(kf.value);
    j["type"] = val["type"];
    j["value"] = val["value"];
    return j;
}

Keyframe keyframeFromJson(const json &j)
{
    Keyframe kf;
    if (j.contains("time") && j["time"].is_number())
    {
        kf.time = j["time"].get<float>();
    }
    if (j.contains("ease"))
    {
        if (j["ease"].is_string())
        {
            kf.ease = easeFromString(j["ease"].get<std::string>());
        }
        else if (j["ease"].is_number_integer())
        {
            kf.ease = static_cast<Ease>(j["ease"].get<int>());
        }
    }
    kf.value = animValueFromJson(j);
    return kf;
}

AnimValue AnimationTrack::evaluate(float time) const
{
    if (keyframes.empty())
    {
        return 0.0f;
    }

    if (keyframes.size() == 1 || time <= keyframes.front().time)
    {
        return keyframes.front().value;
    }

    if (time >= keyframes.back().time)
    {
        return keyframes.back().value;
    }

    size_t idx = 0;
    while (idx + 1 < keyframes.size() && keyframes[idx + 1].time <= time)
    {
        idx++;
    }

    const auto &k0 = keyframes[idx];
    const auto &k1 = keyframes[idx + 1];

    float span = k1.time - k0.time;
    if (span <= 0.0f)
    {
        return k0.value;
    }

    float u = glm::clamp((time - k0.time) / span, 0.0f, 1.0f);

    // Si le type d'easing est None (step / coupure volontaire), conserver la valeur k0 jusqu'à la frame suivante
    if (k0.ease == Ease::None)
    {
        return u >= 1.0f ? k1.value : k0.value;
    }

    // Si un easing élastique est demandé, on applique la formule d'easing directement
    if (isElasticEase(k0.ease))
    {
        float easedFactor = Easing::applyEase(k0.ease, u);
        return interpolateValues(k0.value, k1.value, easedFactor);
    }

    // Si les deux frames sont en Linear et qu'aucune courbure n'est demandée
    if (k0.ease == Ease::Linear && (idx + 1 == keyframes.size() - 1 || keyframes[idx + 1].ease == Ease::Linear))
    {
        return interpolateValues(k0.value, k1.value, u);
    }

    // Calcul des tangentes Hermite (continuité C1 sans coupure aux frames intermédiaires)
    AnimValue m0;
    if (idx == 0)
    {
        if (hasEaseIn(k0.ease))
        {
            m0 = createZeroTangent(k0.value);
        }
        else
        {
            m0 = computeTangentDifference(k0.value, k1.value, 1.0f);
        }
    }
    else
    {
        // Keyframe intermédiaire: tangente continue Catmull-Rom
        const auto &kPrev = keyframes[idx - 1];
        float dt = k1.time - kPrev.time;
        float scale = dt > 0.0001f ? (span / dt) : 1.0f;
        m0 = computeTangentDifference(kPrev.value, k1.value, scale);
    }

    AnimValue m1;
    if (idx + 1 == keyframes.size() - 1)
    {
        if (hasEaseOut(k0.ease) || hasEaseOut(k1.ease))
        {
            m1 = createZeroTangent(k1.value);
        }
        else
        {
            m1 = computeTangentDifference(k0.value, k1.value, 1.0f);
        }
    }
    else
    {
        // Keyframe intermédiaire: tangente continue Catmull-Rom
        const auto &kNext = keyframes[idx + 2];
        float dt = kNext.time - k0.time;
        float scale = dt > 0.0001f ? (span / dt) : 1.0f;
        m1 = computeTangentDifference(k0.value, kNext.value, scale);
    }

    AnimValue v0 = k0.value;
    AnimValue v1 = k1.value;
    if (std::holds_alternative<glm::quat>(v0) && std::holds_alternative<glm::quat>(v1))
    {
        auto q0 = std::get<glm::quat>(v0);
        auto q1 = std::get<glm::quat>(v1);
        if (glm::dot(q0, q1) < 0.0f)
        {
            v1 = -q1;
        }
    }

    return interpolateHermite(v0, v1, m0, m1, u);
}

json trackToJson(const AnimationTrack &track)
{
    json j;
    j["component"] = track.componentName;
    j["property"] = track.propertyName;
    j["keyframes"] = json::array();
    for (const auto &kf : track.keyframes)
    {
        j["keyframes"].push_back(keyframeToJson(kf));
    }
    return j;
}

AnimationTrack trackFromJson(const json &j)
{
    AnimationTrack track;
    if (j.contains("component") && j["component"].is_string())
    {
        track.componentName = j["component"].get<std::string>();
    }
    if (j.contains("property") && j["property"].is_string())
    {
        track.propertyName = j["property"].get<std::string>();
    }
    if (j.contains("keyframes") && j["keyframes"].is_array())
    {
        for (const auto &kfJson : j["keyframes"])
        {
            track.keyframes.push_back(keyframeFromJson(kfJson));
        }
        track.sortKeyframes();
    }
    return track;
}

void Animator::initializeCodec(const Engine &engine)
{
    codec.clear();
    codec.addString("Clip Name", &clipName);
    codec.addBool("Playing", &isPlaying);
    codec.addBool("Loop", &isLooping);
    codec.addFloat("Time", &currentTime);
    codec.addFloat("Duration", &duration);
    codec.addFloat("Speed", &speed);

    tracksCodec.toJson = [this]() -> json {
        return tracksToJson();
    };
    tracksCodec.fromJson = [this](const json &j) {
        tracksFromJson(j);
    };
    codec.addCustomJson("Tracks", &tracksCodec);
}

json Animator::toJson() const
{
    json j = json::object();
    j["Clip Name"] = clipName;
    j["Duration"] = duration;
    j["Speed"] = speed;
    j["Playing"] = isPlaying;
    j["Loop"] = isLooping;
    j["Time"] = currentTime;
    j["Tracks"] = tracksToJson();
    return j;
}

void Animator::fromJson(const json &j)
{
    if (j.contains("Clip Name") && j["Clip Name"].is_string()) clipName = j["Clip Name"].get<std::string>();
    if (j.contains("Duration") && j["Duration"].is_number()) duration = j["Duration"].get<float>();
    if (j.contains("Speed") && j["Speed"].is_number()) speed = j["Speed"].get<float>();
    if (j.contains("Playing") && j["Playing"].is_boolean()) isPlaying = j["Playing"].get<bool>();
    if (j.contains("Loop") && j["Loop"].is_boolean()) isLooping = j["Loop"].get<bool>();
    if (j.contains("Time") && j["Time"].is_number()) currentTime = j["Time"].get<float>();
    if (j.contains("Tracks")) tracksFromJson(j["Tracks"]);
    else if (j.is_array()) tracksFromJson(j);
}

json Animator::tracksToJson() const
{
    json arr = json::array();
    for (const auto &track : tracks)
    {
        arr.push_back(trackToJson(track));
    }
    return arr;
}

void Animator::tracksFromJson(const json &j)
{
    tracks.clear();
    if (j.is_array())
    {
        for (const auto &trackJson : j)
        {
            tracks.push_back(trackFromJson(trackJson));
        }
    }
}
