#ifndef OPENGL_COMPONENT_H
#define OPENGL_COMPONENT_H

#include <optional>

#include "utils/Clonable.h"
#include "utils/Codec.h"


class Engine;
class Entity;
using ComponentId = std::size_t;
constexpr ComponentId INVALID_COMPONENT_ID = 0;


struct ComponentVariant : Clonable
{
    virtual void initializeCodec(const Engine &engine, Codec &codec) {}
};

template<typename VariantType>
struct VariantHolder : Clonable
{
    static void registerVariant(const std::string &name, const std::function<void *()> &creator)
    {
        variants().emplace(name, creator);
    }

    std::string getVariantName() const { return variant != nullptr ? name : "<no variant>"; }

    void setVariant(const std::string &n)
    {
        if (variant != nullptr && name == n) { return; }
        const auto it = variants().find(n);
        if (it != variants().end())
        {
            variant.reset(static_cast<VariantType *>(it->second()));
            this->name = n;
        }
    }

    void initializeCodec(const Engine &engine, Codec &codec)
    {
        if (variant != nullptr) { variant->initializeCodec(engine, codec); }
    }

    static std::vector<std::string> getVariants()
    {
        std::vector<std::string> variantNames;
        for (const auto &key: variants() | std::views::keys) { variantNames.push_back(key); }
        return variantNames;
    }

    VariantType *get() const { return variant.get(); }

    void adopt(VariantType *v, const std::string &n)
    {
        variant.reset(v);
        name = n;
    }

    Clonable *clone() const override { return variant != nullptr ? variant->clone() : nullptr; }

private:
    static std::map<std::string, std::function<void *()> > &variants()
    {
        static std::map<std::string, std::function<void *()> > variants{};
        return variants;
    };

    std::string name;
    std::unique_ptr<VariantType> variant;
};

struct Component : Clonable
{
    Entity *entity{nullptr};
    ComponentId typeId{0};
    bool removed{false};
    std::optional<ComponentId> removedTypeId;
    Codec codec;

    virtual void initializeCodec(const Engine &engine) {}

    virtual bool hasVariant() const { return false; }

    virtual void setVariant(const std::string &name) {};
};

struct UniqueComponent : Component
{
    ~UniqueComponent() override = default;
};

inline ComponentId getNextComponentTypeId()
{
    static ComponentId lastId = 1;
    return lastId++;
}

template<typename T>
ComponentId getComponentTypeId() noexcept
{
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    static const ComponentId id = getNextComponentTypeId();
    return id;
}

#define _CONCAT2(a, b) a##b
#define CONCAT2(a, b) _CONCAT2(a, b)
#define _CONCAT3(a, b, c) a##b##c
#define CONCAT3(a, b, c) _CONCAT3(a, b, c)

#ifndef UNIQUE_ID
#ifdef __COUNTER__
#define UNIQUE_ID __COUNTER__
#else
#define UNIQUE_ID __LINE__
#endif
#endif

namespace detail
{
template<typename Holder>
struct Registrar
{
    Registrar(const char *name, std::function<void*()> creator) { Holder::registerVariant(name, std::move(creator)); }
};
}

#define REGISTER_VARIANT(Holder, Name, Type) \
    namespace { \
        const ::detail::Registrar<Holder> CONCAT3(_reg_, Holder, UNIQUE_ID)(Name, [] { return new Type(); }); \
    }

#endif //OPENGL_COMPONENT_H
