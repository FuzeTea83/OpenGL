#include "Archetype.h"

Archetype::Archetype(Signature signature) : mSignature(std::move(signature)) {}

void Archetype::addEntity(Entity *entity, std::vector<std::unique_ptr<Component> > &&components)
{
    mEntities.push_back(entity);
    mComponents.push_back(std::move(components));
}

void Archetype::removeEntity(const Entity &entity)
{
    const std::size_t index = getIndex(entity);
    if (index == -1) { return; }

    mEntities.erase(mEntities.begin() + index);
    mComponents.erase(mComponents.begin() + index);
}

std::vector<std::unique_ptr<Component> > Archetype::removeAndExtractComponents(const Entity &entity)
{
    const std::size_t index = getIndex(entity);
    if (index == -1) { return {}; }

    auto comp = std::move(mComponents[index]);
    mEntities.erase(mEntities.begin() + index);
    mComponents.erase(mComponents.begin() + index);

    return comp;
}

std::vector<std::unique_ptr<Component> > &Archetype::getComponents(const Entity &entity)
{
    const std::size_t index = getIndex(entity);
    if (index == -1) { throw std::runtime_error("Entity not found in archetype"); }
    return mComponents[index];
}

void Archetype::refreshEntities()
{
    for (std::size_t i = 0; i < mEntities.size();)
    {
        if (mEntities[i]->isRemoved())
        {
            mEntities[i]->mArchetype = nullptr;
            mEntities.erase(mEntities.begin() + i);
            mComponents.erase(mComponents.begin() + i);
        }
        else { ++i; }
    }
}

std::size_t Archetype::getIndex(const Entity &entity) const
{
    for (std::size_t i = 0; i < mEntities.size(); ++i) { if (mEntities[i]->getId() == entity.getId()) { return i; } }
    return -1;
}

