#ifndef OPENGL_ENTITY_H
#define OPENGL_ENTITY_H

#include <string>
#include <vector>
#include "Component.h"

class ECS;
class Archetype;

using EntityId = unsigned int;

class Entity
{
public:
    Entity(ECS &ecs, const std::uint32_t id, const std::uint32_t version) : mEcs(ecs), mId(id), mVersion(version) {}

    Entity(const Entity &) = default;

    EntityId getId() const { return mId; }

    bool isRemoved() const { return mRemoved; }

    void remove() { mRemoved = true; }

    template<typename T, typename... A>
    T &addComponent(A &&... args);

    Component &addComponent(ComponentId id);

    template<typename T>
    bool hasComponent() const;

    bool hasComponent(ComponentId id) const;

    template<typename T>
    T &getComponent();

    template<typename T>
    const T &getComponent() const;

    template<typename T>
    const T *tryGetComponent() const;

    std::vector<Component *> getComponents() const;

    template<typename T>
    void removeComponent();

    void removeComponent(ComponentId id) const;

    bool operator==(const Entity &other) const { return mId == other.mId; }

    std::string &getName() { return mName; }

    std::string getName() const { return mName; }

    void setName(const std::string &name) { mName = name; }

    bool isVisible() const { return mVisible; }

    void setVisible(const bool visible) { mVisible = visible; }

    std::shared_ptr<Entity> clone() const;

private:
    friend class ECS;
    friend class Archetype;
    ECS &mEcs;
    std::uint32_t mId;
    std::uint32_t mVersion;
    std::string mName;
    bool mVisible = true;
    bool mRemoved = false;
    Archetype *mArchetype = nullptr;
};
#endif //OPENGL_ENTITY_H
