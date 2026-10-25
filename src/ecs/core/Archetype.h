#ifndef OPENGL_ARCHETYPE_H
#define OPENGL_ARCHETYPE_H

#include <memory>
#include <vector>

#include "Component.h"
#include "Entity.h"
#include "Signature.h"

class Archetype
{
public:
    explicit Archetype(Signature signature);

private:
    friend class ECS;

    void addEntity(Entity *entity, std::vector<std::unique_ptr<Component> > &&components);

    void removeEntity(const Entity &entity);

    std::vector<std::unique_ptr<Component> > removeAndExtractComponents(const Entity &entity);

    std::vector<std::unique_ptr<Component> > &getComponents(const Entity &entity);

    void refreshEntities();

    std::size_t getIndex(const Entity &entity) const;

    const Signature &getSignature() const { return mSignature; }

    Signature mSignature;
    std::vector<Entity *> mEntities;
    std::vector<std::vector<std::unique_ptr<Component> > > mComponents;
};


#endif //OPENGL_ARCHETYPE_H
