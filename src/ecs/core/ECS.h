#ifndef OPENGL_ECS_H
#define OPENGL_ECS_H

#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "Archetype.h"
#include "Component.h"
#include "Entity.h"
#include "Signature.h"
#include "System.h"

class Engine;
using EntityAddedCallback = std::function<void(const std::shared_ptr<Entity> &)>;
using EntityRemovedCallback = std::function<void(const std::shared_ptr<Entity> &)>;

class ECS
{
public:
    explicit ECS(const Engine &engine) : mEngine(engine) {}

    std::shared_ptr<Entity> &createEntity(const std::string &name = "");

    std::shared_ptr<Entity> &cloneEntity(const Entity *source);

    std::vector<std::shared_ptr<Entity> > getEntities() const;

    template<typename T>
    void registerComponent(const std::string &name);

    template<typename T>
    std::string &getComponentName() const;

    std::string &getComponentName(ComponentId id);

    ComponentId getComponentIdByName(const std::string &string);

    std::map<ComponentId, std::string> getComponentTypes();

    template<typename T, typename... TArgs>
    T &addComponent(Entity &entity, TArgs &&... args);

    Component &addComponent(Entity &entity, ComponentId id);

    template<typename T>
    bool hasComponent(const Entity &entity) const;

    bool hasComponent(const Entity &entity, ComponentId id) const;

    template<typename T>
    T &getComponent(Entity &entity);

    template<typename T>
    T &getComponent(Entity &entity) const;

    template<typename T>
    T *tryGetComponent(Entity &entity) const;

    std::vector<Component *> getComponents(const Entity &entity) const;

    template<typename T>
    void removeComponent(Entity &entity) const;

    void removeComponent(const Entity &entity, ComponentId id) const;

    template<class... Ts>
    std::vector<Entity *> query() const;

    void refreshEntities();

    void update(float deltaTime) const;

    void render(float alpha) const;

    template<typename T, typename... TArgs>
    T *createSystem(unsigned int priority = 0, TArgs &&... args);

    template<typename T, typename TSystem>
    T *getSystem(const std::vector<std::unique_ptr<TSystem> > &systems);

    template<typename T>
    void destroySystem();

    void clearEntities();

    void clearAll();

    bool canInterpretInputs() const { return mInterpretInputs; }

    void setInterpretInputs(const bool value) { mInterpretInputs = value; }

    void addEntityAddedCallback(const EntityAddedCallback &callback) { mEntityAddedCallbacks.push_back(callback); }

    void addEntityRemovedCallback(const EntityRemovedCallback &callback)
    {
        mEntityRemovedCallbacks.push_back(callback);
    }

private:
    Archetype *getArchetype(const Signature &signature) const;

    Archetype &getOrCreateArchetype(const Signature &signature);

    const Engine &mEngine;

    mutable std::recursive_mutex mMutex;

    std::vector<uint32_t> mEntityVersions;
    std::queue<uint32_t> mFreeEntityIds;

    std::vector<std::unique_ptr<Archetype> > mArchetypes;
    std::vector<std::shared_ptr<Entity> > mAllEntities;
    std::vector<std::unique_ptr<UniqueComponent> > mUniqueComponents;
    std::map<ComponentId, std::pair<std::string, std::function<std::unique_ptr<Component>()> > > mComponentTypes;

    std::vector<std::unique_ptr<IUpdateSystem> > mUpdateSystems;
    std::vector<std::unique_ptr<IRenderSystem> > mRenderSystems;

    bool mInterpretInputs = true;

    std::vector<EntityAddedCallback> mEntityAddedCallbacks;
    std::vector<EntityRemovedCallback> mEntityRemovedCallbacks;
};

template<typename T>
void ECS::registerComponent(const std::string &name)
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    const ComponentId id = getComponentTypeId<T>();
    mComponentTypes[id] = std::pair<std::string, std::function<std::unique_ptr<Component>()> >(
        name, []() -> std::unique_ptr<Component>
        {
            if constexpr (std::is_default_constructible_v<T>) { return std::make_unique<T>(); }
            else { throw std::runtime_error("Component cannot be created without constructor arguments"); }
        });
}

template<typename T>
std::string &ECS::getComponentName() const { return getComponentName(getComponentTypeId<T>()); }

template<typename T, typename... TArgs>
T &ECS::addComponent(Entity &entity, TArgs &&... args)
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    static_assert(!std::is_base_of_v<UniqueComponent, T>, "T can not be derived from UniqueComponent");

    auto c = std::make_unique<T>(std::forward<TArgs>(args)...);
    c->entity = &entity;
    c->typeId = getComponentTypeId<T>();
    T &ref = *c;

    Signature newSig;
    std::vector<std::unique_ptr<Component> > comps;
    if (entity.mArchetype != nullptr)
    {
        newSig = entity.mArchetype->getSignature();
        newSig.setComponent<T>(true);
        comps = entity.mArchetype->removeAndExtractComponents(entity);
    }
    else { newSig = Signature::create<T>(); }

    comps.push_back(std::move(c));

    auto &newArchetype = getOrCreateArchetype(newSig);
    entity.mArchetype = &newArchetype;
    newArchetype.addEntity(&entity, std::move(comps));
    return ref;
}

template<typename T>
bool ECS::hasComponent(const Entity &entity) const { return hasComponent(entity, getComponentTypeId<T>()); }


template<typename T>
T &ECS::getComponent(Entity &entity)
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    if (entity.mArchetype == nullptr) { throw std::runtime_error("Entity has no components/archetype"); }

    for (auto &comp: entity.mArchetype->getComponents(entity))
    {
        if (auto casted = dynamic_cast<T *>(comp.get())) { return *casted; }
    }

    throw std::runtime_error("Component not found on entity");
}

template<typename T>
T &ECS::getComponent(Entity &entity) const
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    if (entity.mArchetype == nullptr) { throw std::runtime_error("Entity has no components/archetype"); }

    for (auto &comp: entity.mArchetype->getComponents(entity))
    {
        if (auto casted = dynamic_cast<T *>(comp.get())) { return *casted; }
    }

    throw std::runtime_error("Component not found on entity");
}

template<typename T>
T *ECS::tryGetComponent(Entity &entity) const
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    if (entity.mArchetype == nullptr) { return nullptr; }

    for (auto &comp: entity.mArchetype->getComponents(entity))
    {
        if (auto casted = dynamic_cast<T *>(comp.get())) { return casted; }
    }

    return nullptr;
}


template<typename T>
void ECS::removeComponent(Entity &entity) const
{
    std::lock_guard lock(mMutex);
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    if (entity.mArchetype == nullptr) { return; }

    if (!entity.mArchetype->getSignature().hasComponent<T>()) { return; }

    for (const auto &component: entity.mArchetype->getComponents(entity))
    {
        if (getComponentTypeId<T>() != component->typeId) { continue; }
        component->removed = true;
        component->removedTypeId = getComponentTypeId<T>();
        return;
    }
}


template<typename... Ts>
std::vector<Entity *> ECS::query() const
{
    std::lock_guard lock(mMutex);
    std::vector<Entity *> entities;
    const Signature querySig = Signature::create<Ts...>();

    for (const auto &archetype: mArchetypes)
    {
        if (!archetype->getSignature().matches(querySig)) { continue; }

        for (const auto &entity: archetype->mEntities) { if (!entity->isRemoved()) { entities.push_back(entity); } }
    }


    return entities;
}

template<typename T, typename... TArgs>
T *ECS::createSystem(unsigned int priority, TArgs &&... args)
{
    static_assert(std::is_base_of_v<System, T>, "T must derive from System");

    if constexpr (std::is_base_of_v<IUpdateSystem, T>)
    {
        if (getSystem<T>(mUpdateSystems) != nullptr) { throw std::runtime_error("System already exists"); }

        auto system = std::make_unique<T>(priority, std::forward<TArgs>(args)...);
        T *ptr = system.get();

        std::lock_guard lock(mMutex);
        const auto it = std::lower_bound(mUpdateSystems.begin(), mUpdateSystems.end(), priority,
                                         [](const std::unique_ptr<IUpdateSystem> &p, const Priority x)
                                         {
                                             return p->getPriority() < x;
                                         });

        std::unique_ptr<IUpdateSystem> baseSystem = std::move(system);
        mUpdateSystems.insert(it, std::move(baseSystem));
        return ptr;
    }
    else if constexpr (std::is_base_of_v<IRenderSystem, T>)
    {
        if (getSystem<T>(mRenderSystems) != nullptr) { throw std::runtime_error("System already exists"); }

        auto system = std::make_unique<T>(priority, std::forward<TArgs>(args)...);
        T *ptr = system.get();

        std::lock_guard lock(mMutex);
        const auto it = std::lower_bound(mRenderSystems.begin(), mRenderSystems.end(), priority,
                                         [](const std::unique_ptr<IRenderSystem> &p, const Priority x)
                                         {
                                             return p->getPriority() < x;
                                         });

        std::unique_ptr<IRenderSystem> baseSystem = std::move(system);
        mRenderSystems.insert(it, std::move(baseSystem));
        return ptr;
    }
    else
    {
        static_assert(!sizeof(T *), "System must derive from either IUpdateSystem or IRenderSystem");
        return nullptr;
    }
}

template<typename T, typename TSystem>
T *ECS::getSystem(const std::vector<std::unique_ptr<TSystem> > &systems)
{
    static_assert(std::is_base_of_v<System, T>, "T must derive from System");
    std::lock_guard lock(mMutex);
    for (auto &s: systems) { if (auto *casted = dynamic_cast<T *>(s.get())) { return casted; } }
    return nullptr;
}

template<typename T>
void ECS::destroySystem()
{
    static_assert(std::is_base_of_v<System, T>, "T must derive from System");
    if constexpr (std::is_base_of_v<IUpdateSystem, T>)
    {
        std::lock_guard lock(mMutex);
        std::erase_if(mUpdateSystems, [](const auto &s) { return dynamic_cast<T *>(s.get()) != nullptr; });
    }
    else if constexpr (std::is_base_of_v<IRenderSystem, T>)
    {
        std::lock_guard lock(mMutex);
        std::erase_if(mRenderSystems, [](const auto &s) { return dynamic_cast<T *>(s.get()) != nullptr; });
    }
}


template<typename T, typename... TArgs>
T &Entity::addComponent(TArgs &&... args) { return mEcs.addComponent<T>(*this, std::forward<TArgs>(args)...); }

inline Component &Entity::addComponent(const ComponentId id) { return mEcs.addComponent(*this, id); }

template<typename T>
bool Entity::hasComponent() const { return mEcs.hasComponent<T>(*this); }

inline bool Entity::hasComponent(const ComponentId id) const { return mEcs.hasComponent(*this, id); }

template<typename T>
T &Entity::getComponent() { return mEcs.getComponent<T>(*this); }

template<typename T>
const T &Entity::getComponent() const { return mEcs.getComponent<T>(*this); }

template<typename T>
const T *Entity::tryGetComponent() const { return mEcs.tryGetComponent<T>(*this); }

inline std::vector<Component *> Entity::getComponents() const { return mEcs.getComponents(*this); }

template<typename T>
void Entity::removeComponent() { mEcs.removeComponent<T>(*this); }

inline void Entity::removeComponent(const ComponentId id) const { mEcs.removeComponent(*this, id); }

inline std::shared_ptr<Entity> Entity::clone() const { return mEcs.cloneEntity(this); }

#endif //OPENGL_ECS_H
