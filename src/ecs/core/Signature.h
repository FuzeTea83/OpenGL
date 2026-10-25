#ifndef OPENGL_SIGNATURE_H
#define OPENGL_SIGNATURE_H

#include <vector>
#include <algorithm>

#include "Component.h"

class Signature
{
public:
    template<typename... Ts>
    static Signature create()
    {
        Signature result;
        (result.setComponent<Ts>(true), ...);
        return result;
    }

    template<typename T>
    void setComponent(const bool value) { setComponent(getComponentTypeId<T>(), value); }

    void setComponent(const ComponentId id, const bool value)
    {
        const std::size_t index = id - 1;
        if (index >= mBits.size()) { mBits.resize(index + 1, false); }
        mBits[index] = value;
    }

    template<typename T>
    bool hasComponent() const { return hasComponent(getComponentTypeId<T>()); }

    bool hasComponent(const ComponentId id) const
    {
        const std::size_t index = id - 1;
        if (index >= mBits.size()) { return false; }
        return mBits.at(index);
    }

    bool operator==(const Signature &other) const
    {
        const std::size_t maxSize = std::max(mBits.size(), other.mBits.size());
        const std::size_t minSize = std::min(mBits.size(), other.mBits.size());
        const Signature big = maxSize == mBits.size() ? *this : other;

        for (std::size_t i = 0; i < minSize; ++i) { if (other.mBits[i] && !mBits[i]) return false; }
        for (std::size_t i = minSize; i < maxSize; ++i) { if (big.mBits[i]) return false; }

        return true;
    }

    bool matches(const Signature &other) const
    {
        for (std::size_t i = 0; i < other.mBits.size(); ++i)
        {
            if (other.mBits[i] && (i >= mBits.size() || !mBits[i])) return false;
        }

        return true;
    }

    const std::vector<bool> &getBits() const { return mBits; }

private:
    friend class ECS;

    Signature() = default;

    std::vector<bool> mBits{};
};

#endif //OPENGL_SIGNATURE_H
