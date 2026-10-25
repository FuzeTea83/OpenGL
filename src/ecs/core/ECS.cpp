#include "ECS.h"

#include <algorithm>
#include <iostream>

#include "core/Engine.h"
#include "utils/OpenGLUtils.h"


std::shared_ptr<Entity> &ECS::createEntity(const std::string &name)
{
    std::lock_guard lock(mMutex);
    std::uint32_t id;
    if (mFreeEntityIds.empty())
    {
        id = static_cast<std::uint32_t>(mAllEntities.size());
        mAllEntities.emplace_back();
        mEntityVersions.push_back(1);
    }
    else
    {
        id = mFreeEntityIds.front();
        mFreeEntityIds.pop();
    }
    auto entity = std::make_shared<Entity>(*this, id, mEntityVersions[id]);
    entity->setName(std::move(name));
    mAllEntities[id] = std::move(entity);
    for (const auto &callback: mEntityAddedCallbacks) { callback(mAllEntities[id]); }
    return mAllEntities[id];
}

std::shared_ptr<Entity> &ECS::cloneEntity(const Entity* source)
{
    std::shared_ptr<Entity> &clone = createEntity(source->getName());
    std::vector<std::unique_ptr<Component> > components;
    for (const auto &component: source->getComponents())
    {
        const auto c = dynamic_cast<Component *>(component->clone());
        c->entity = clone.get();
        c->typeId = component->typeId;
        std::unique_ptr<Component> ptr(c);
        components.push_back(std::move(ptr));
    }
    clone->mArchetype = source->mArchetype;
    clone->mVisible = source->mVisible;
    source->mArchetype->addEntity(clone.get(), std::move(components));

    return clone;
}

std::vector<std::shared_ptr<Entity>> ECS::getEntities() const
{
    std::lock_guard lock(mMutex);
    std::vector<std::shared_ptr<Entity>> result;
    result.reserve(mAllEntities.size());
    for (const auto &entity: mAllEntities) { if (entity && !entity->isRemoved()) { result.push_back(entity); } }
    return result;
}

std::string &ECS::getComponentName(const ComponentId id)
{
    std::lock_guard lock(mMutex);
    return mComponentTypes.at(id).first;
}

ComponentId ECS::getComponentIdByName(const std::string &string)
{
    std::lock_guard lock(mMutex);
    for (const auto &[id, pair]: mComponentTypes) { if (pair.first == string) { return id; } }
    return INVALID_COMPONENT_ID;
}

std::map<ComponentId, std::string> ECS::getComponentTypes()
{
    std::lock_guard lock(mMutex);
    std::map<ComponentId, std::string> result;
    for (const auto &[id, pair]: mComponentTypes) { result[id] = pair.first; }
    return result;
}


Component &ECS::addComponent(Entity &entity, const ComponentId id)
{
    std::lock_guard lock(mMutex);
    const auto it = mComponentTypes.find(id);
    if (it == mComponentTypes.end()) { throw std::runtime_error("Component type not registered"); }

    auto c = it->second.second();
    c->entity = &entity;
    c->typeId = id;
    Component &ref = *c;

    Signature newSig;
    std::vector<std::unique_ptr<Component> > comps;
    if (entity.mArchetype != nullptr)
    {
        newSig = entity.mArchetype->getSignature();
        newSig.setComponent(id, true);
        comps = entity.mArchetype->removeAndExtractComponents(entity);
    }
    else
    {
        newSig = Signature();
        newSig.setComponent(id, true);
    }

    comps.push_back(std::move(c));

    auto &newArchetype = getOrCreateArchetype(newSig);
    entity.mArchetype = &newArchetype;
    newArchetype.addEntity(&entity, std::move(comps));
    return ref;
}


bool ECS::hasComponent(const Entity &entity, const ComponentId id) const
{
    std::lock_guard lock(mMutex);
    return entity.mArchetype != nullptr && entity.mArchetype->getSignature().hasComponent(id);
}


std::vector<Component *> ECS::getComponents(const Entity &entity) const
{
    std::lock_guard lock(mMutex);
    if (entity.mArchetype == nullptr) { return {}; }

    std::vector<Component *> result;
    for (const auto &comp: entity.mArchetype->getComponents(entity)) { result.push_back(comp.get()); }
    return result;
}


void ECS::removeComponent(const Entity &entity, const ComponentId id) const
{
    std::lock_guard lock(mMutex);
    if (entity.mArchetype == nullptr) { return; }

    if (!entity.mArchetype->getSignature().hasComponent(id)) { return; }

    for (const auto &component: entity.mArchetype->getComponents(entity))
    {
        if (id != component->typeId) { continue; }
        component->removed = true;
        component->removedTypeId = id;
        return;
    }
}

void ECS::refreshEntities()
{
    std::lock_guard lock(mMutex);
    for (const auto &entity: mAllEntities)
    {
        if (!entity) { continue; }
        if (entity->isRemoved())
        {
            for (const auto &callback: mEntityRemovedCallbacks) { callback(entity); }
            continue;
        }
        if (entity->mArchetype == nullptr) { continue; }

        auto &archetype = *entity->mArchetype;
        const Signature oldSignature = archetype.getSignature();
        std::vector<std::unique_ptr<Component> > components = archetype.removeAndExtractComponents(*entity);
        Signature newSignature = oldSignature;

        std::erase_if(components, [&newSignature](const std::unique_ptr<Component> &component)
        {
            if (!component->removed) { return false; }
            if (component->removedTypeId) { newSignature.setComponent(*component->removedTypeId, false); }
            return true;
        });

        if (components.empty())
        {
            entity->mArchetype = nullptr;
            continue;
        }

        auto &newArchetype = getOrCreateArchetype(newSignature);
        entity->mArchetype = &newArchetype;
        newArchetype.addEntity(entity.get(), std::move(components));
    }

    for (const auto &archetype: mArchetypes) { archetype->refreshEntities(); }


    for (std::uint32_t id = 0; id < mAllEntities.size(); ++id)
    {
        if (!mAllEntities[id] || !mAllEntities[id]->isRemoved()) { continue; }
        ++mEntityVersions[id];
        mFreeEntityIds.push(id);
        mAllEntities[id].reset();
    }
}


void ECS::update(const float deltaTime) const
{
    std::lock_guard lock(mMutex);
    for (const auto &system: mUpdateSystems)
    {
        const Signature &signature = system->getSignature();
        system->start(mEngine);
        for (const auto &archetype: mArchetypes)
        {
            if (!archetype->getSignature().matches(signature)) { continue; }
            for (const auto &entity : archetype->mEntities)
            {
                if (entity->isVisible() && !entity->isRemoved()) { system->update(mEngine, *entity, deltaTime); }
            }
        }
        system->stop(mEngine);
    }
}


void ECS::render(const float alpha) const
{
    assertGlThread();
    std::lock_guard lock(mMutex);
    for (const auto &system: mRenderSystems)
    {
        system->start(mEngine);
        const Signature &signature = system->getSignature();
        for (const auto &archetype: mArchetypes)
        {
            if (!archetype->getSignature().matches(signature)) { continue; }
            for (const auto &entity: archetype->mEntities)
            {
                if (entity->isVisible() && !entity->isRemoved()) { system->render(mEngine, *entity, alpha); }
            }
        }
        system->stop(mEngine);
    }
}

void ECS::clearEntities()
{
    std::lock_guard lock(mMutex);
    mArchetypes.clear();
    for (std::uint32_t id = 0; id < mAllEntities.size(); ++id)
    {
        if (!mAllEntities[id]) { continue; }
        ++mEntityVersions[id];
        mAllEntities[id].reset();
        mFreeEntityIds.push(id);
    }
}

void ECS::clearAll()
{
    std::lock_guard lock(mMutex);
    mArchetypes.clear();
    for (std::uint32_t id = 0; id < mAllEntities.size(); ++id)
    {
        if (!mAllEntities[id]) { continue; }
        ++mEntityVersions[id];
        mAllEntities[id].reset();
        mFreeEntityIds.push(id);
    }
    mUpdateSystems.clear();
    mRenderSystems.clear();
    mUniqueComponents.clear();
}

Archetype *ECS::getArchetype(const Signature &signature) const
{
    for (const auto &archetype: mArchetypes)
    {
        if (archetype->getSignature().getBits() == signature.getBits()) { return archetype.get(); }
    }
    return nullptr;
}

Archetype &ECS::getOrCreateArchetype(const Signature &signature)
{
    if (auto *existing = getArchetype(signature)) { return *existing; }
    mArchetypes.push_back(std::make_unique<Archetype>(signature));
    return *mArchetypes.back();
}
