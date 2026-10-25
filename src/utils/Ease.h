#ifndef OPENGL_EASE_H
#define OPENGL_EASE_H

#include <cmath>
#include <string>

enum class Ease
{
    None,
    Linear,
    InSine,
    OutSine,
    InOutSine,
    InCubic,
    OutCubic,
    InOutCubic,
    InQuint,
    OutQuint,
    InOutQuint,
    InCirc,
    OutCirc,
    InOutCirc,
    InElastic,
    OutElastic,
    InOutElastic,
    InQuad,
    OutQuad,
    InOutQuad,
    InQuart,
    OutQuart,
    InOutQuart,
    InExpo,
    OutExpo,
    InOutExpo
};

inline const char *easeToString(Ease ease)
{
    switch (ease)
    {
        case Ease::None: return "None";
        case Ease::Linear: return "Linear";
        case Ease::InSine: return "InSine";
        case Ease::OutSine: return "OutSine";
        case Ease::InOutSine: return "InOutSine";
        case Ease::InQuad: return "InQuad";
        case Ease::OutQuad: return "OutQuad";
        case Ease::InOutQuad: return "InOutQuad";
        case Ease::InCubic: return "InCubic";
        case Ease::OutCubic: return "OutCubic";
        case Ease::InOutCubic: return "InOutCubic";
        case Ease::InQuart: return "InQuart";
        case Ease::OutQuart: return "OutQuart";
        case Ease::InOutQuart: return "InOutQuart";
        case Ease::InQuint: return "InQuint";
        case Ease::OutQuint: return "OutQuint";
        case Ease::InOutQuint: return "InOutQuint";
        case Ease::InExpo: return "InExpo";
        case Ease::OutExpo: return "OutExpo";
        case Ease::InOutExpo: return "InOutExpo";
        case Ease::InCirc: return "InCirc";
        case Ease::OutCirc: return "OutCirc";
        case Ease::InOutCirc: return "InOutCirc";
        case Ease::InElastic: return "InElastic";
        case Ease::OutElastic: return "OutElastic";
        case Ease::InOutElastic: return "InOutElastic";
        default: return "None";
    }
}

inline Ease easeFromString(const std::string &str)
{
    if (str == "None") return Ease::None;
    if (str == "InSine") return Ease::InSine;
    if (str == "OutSine") return Ease::OutSine;
    if (str == "InOutSine") return Ease::InOutSine;
    if (str == "InQuad") return Ease::InQuad;
    if (str == "OutQuad") return Ease::OutQuad;
    if (str == "InOutQuad") return Ease::InOutQuad;
    if (str == "InCubic") return Ease::InCubic;
    if (str == "OutCubic") return Ease::OutCubic;
    if (str == "InOutCubic") return Ease::InOutCubic;
    if (str == "InQuart") return Ease::InQuart;
    if (str == "OutQuart") return Ease::OutQuart;
    if (str == "InOutQuart") return Ease::InOutQuart;
    if (str == "InQuint") return Ease::InQuint;
    if (str == "OutQuint") return Ease::OutQuint;
    if (str == "InOutQuint") return Ease::InOutQuint;
    if (str == "InExpo") return Ease::InExpo;
    if (str == "OutExpo") return Ease::OutExpo;
    if (str == "InOutExpo") return Ease::InOutExpo;
    if (str == "InCirc") return Ease::InCirc;
    if (str == "OutCirc") return Ease::OutCirc;
    if (str == "InOutCirc") return Ease::InOutCirc;
    if (str == "InElastic") return Ease::InElastic;
    if (str == "OutElastic") return Ease::OutElastic;
    if (str == "InOutElastic") return Ease::InOutElastic;
    return Ease::None;
}

inline bool hasEaseIn(const Ease ease)
{
    switch (ease)
    {
        case Ease::InSine:
        case Ease::InOutSine:
        case Ease::InQuad:
        case Ease::InOutQuad:
        case Ease::InCubic:
        case Ease::InOutCubic:
        case Ease::InQuart:
        case Ease::InOutQuart:
        case Ease::InQuint:
        case Ease::InOutQuint:
        case Ease::InExpo:
        case Ease::InOutExpo:
        case Ease::InCirc:
        case Ease::InOutCirc:
        case Ease::InElastic:
        case Ease::InOutElastic:
            return true;
        default:
            return false;
    }
}

inline bool hasEaseOut(const Ease ease)
{
    switch (ease)
    {
        case Ease::OutSine:
        case Ease::InOutSine:
        case Ease::OutQuad:
        case Ease::InOutQuad:
        case Ease::OutCubic:
        case Ease::InOutCubic:
        case Ease::OutQuart:
        case Ease::InOutQuart:
        case Ease::OutQuint:
        case Ease::InOutQuint:
        case Ease::OutExpo:
        case Ease::InOutExpo:
        case Ease::OutCirc:
        case Ease::InOutCirc:
        case Ease::OutElastic:
        case Ease::InOutElastic:
            return true;
        default:
            return false;
    }
}

inline bool isElasticEase(const Ease ease)
{
    return ease == Ease::InElastic || ease == Ease::OutElastic || ease == Ease::InOutElastic;
}

class Easing
{
public:
    static float applyEase(const Ease ease, const float t)
    {
        switch (ease)
        {
            case Ease::None: return 0.0f;
            case Ease::Linear: return easeLinear(t);
            case Ease::InSine: return easeInSine(t);
            case Ease::OutSine: return easeOutSine(t);
            case Ease::InOutSine: return easeInOutSine(t);
            case Ease::InCubic: return easeInCubic(t);
            case Ease::OutCubic: return easeOutCubic(t);
            case Ease::InOutCubic: return easeInOutCubic(t);
            case Ease::InQuint: return easeInQuint(t);
            case Ease::OutQuint: return easeOutQuint(t);
            case Ease::InOutQuint: return easeInOutQuint(t);
            case Ease::InCirc: return easeInCirc(t);
            case Ease::OutCirc: return easeOutCirc(t);
            case Ease::InOutCirc: return easeInOutCirc(t);
            case Ease::InElastic: return easeInElastic(t);
            case Ease::OutElastic: return easeOutElastic(t);
            case Ease::InOutElastic: return easeInOutElastic(t);
            case Ease::InQuad: return easeInQuad(t);
            case Ease::OutQuad: return easeOutQuad(t);
            case Ease::InOutQuad: return easeInOutQuad(t);
            case Ease::InQuart: return easeInQuart(t);
            case Ease::OutQuart: return easeOutQuart(t);
            case Ease::InOutQuart: return easeInOutQuart(t);
            case Ease::InExpo: return easeInExpo(t);
            case Ease::OutExpo: return easeOutExpo(t);
            case Ease::InOutExpo: return easeInOutExpo(t);
            default: return 0.0f;
        }
    }

    static float easeLinear(const float t) { return t; }

    static float easeInSine(const float t) { return 1 - std::cos(t * M_PI / 2); }

    static float easeOutSine(const float t) { return std::sin(t * M_PI / 2); }

    static float easeInOutSine(const float t) { return -(std::cos(M_PI * t) - 1) / 2; }

    static float easeInCubic(const float t) { return t * t * t; }

    static float easeOutCubic(const float t) { return 1 - std::pow(1 - t, 3); }

    static float easeInOutCubic(const float t) { return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; }

    static float easeInQuint(const float t) { return t * t * t * t * t; }

    static float easeOutQuint(const float t) { return 1 - std::pow(1 - t, 5); }

    static float easeInOutQuint(const float t)
    {
        return t < 0.5f ? 16 * t * t * t * t * t : 1 - std::pow(-2 * t + 2, 5) / 2;
    }

    static float easeInCirc(const float t) { return 1 - std::sqrt(1 - std::pow(t, 2)); }

    static float easeOutCirc(const float t) { return std::sqrt(1 - std::pow(t - 1, 2)); }

    static float easeInOutCirc(const float t)
    {
        return t < 0.5f
                   ? (1 - std::sqrt(1 - std::pow(2 * t, 2))) / 2
                   : (std::sqrt(1 - std::pow(-2 * t + 2, 2)) + 1) / 2;
    }

    static float easeInElastic(const float t)
    {
        constexpr float c4 = 2 * M_PI / 3;
        return t == 0
                   ? 0
                   : t == 1
                         ? 1
                         : -std::pow(2, 10 * t - 10) * std::sin((t * 10 - 10.75) * c4);
    }

    static float easeOutElastic(const float t)
    {
        constexpr float c4 = 2 * M_PI / 3;
        return t == 0
                   ? 0
                   : t == 1
                         ? 1
                         : std::pow(2, -10 * t) * std::sin((t * 10 - 0.75) * c4) + 1;
    }

    static float easeInOutElastic(const float t)
    {
        constexpr float c5 = 2 * M_PI / 4.5;
        return t == 0
                   ? 0
                   : t == 1
                         ? 1
                         : t < 0.5
                               ? -(std::pow(2, 20 * t - 10) *
                                   std::sin((20 * t - 11.125) * c5)) /
                                 2
                               : std::pow(2, -20 * t + 10) *
                                 std::sin((20 * t - 11.125) * c5) / 2 +
                                 1;
    }

    static float easeInQuad(const float t) { return t * t; }

    static float easeOutQuad(const float t) { return 1 - (1 - t) * (1 - t); }
    static float easeInOutQuad(const float t) { return t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2) / 2; }

    static float easeInQuart(const float t) { return t * t * t * t; }

    static float easeOutQuart(const float t) { return 1 - std::pow(1 - t, 4); }

    static float easeInOutQuart(const float t)
    {
        return t < 0.5f ? 8 * t * t * t * t : 1 - std::pow(-2 * t + 2, 4) / 2;
    }

    static float easeInExpo(const float t) { return t == 0 ? 0 : std::pow(2, 10 * t - 10); }

    static float easeOutExpo(const float t) { return t == 1 ? 1 : 1 - std::pow(2, -10 * t); }

    static float easeInOutExpo(const float t)
    {
        return t == 0
                   ? 0
                   : t == 1
                         ? 1
                         : t < 0.5
                               ? std::pow(2, 20 * t - 10) / 2
                               : (2 - std::pow(2, -20 * t + 10)) / 2;
    }
};

#endif //OPENGL_EASE_H
