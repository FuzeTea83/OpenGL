#ifndef OPENGL_COLLISION_COMPONENT_H
#define OPENGL_COLLISION_COMPONENT_H
/*
#include <functional>
#include <variant>
#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include "ecs/core/Component.h"
#include "resource/mesh/Mesh.h"

struct AABB
{
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct Sphere
{
    glm::vec3 center{0.0f};
    float radius{0.5f};
};

struct Capsule
{
    glm::vec3 a{0.0f, 0.0f, 0.0f};
    glm::vec3 b{0.0f, 1.0f, 0.0f};
    float radius{0.5f};
};

struct Plane
{
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float d{0.0f};
};

using ColliderShape = std::variant<AABB, Sphere, Capsule, Plane>;

class Entity;

class ColliderComponent : public Component
{
public:
    explicit ColliderComponent(const Mesh *mesh, const bool trigger = false, const float weight = 1.0f)
        : mTrigger(trigger), mWeight(weight)
    {
        if (!mesh || mesh->subMeshes.empty())
        {
            mShape = AABB{.min = glm::vec3(-0.5f), .max = glm::vec3(0.5f)};
            return;
        }

        bool firstVertex = true;
        glm::vec3 min(0.0f);
        glm::vec3 max(0.0f);

        for (const auto &group: mesh->subMeshes)
        {
            for (const auto &vertex: group.vertices)
            {
                if (firstVertex)
                {
                    min = vertex.position;
                    max = vertex.position;
                    firstVertex = false;
                }
                else
                {
                    min = glm::min(min, vertex.position);
                    max = glm::max(max, vertex.position);
                }
            }
        }

        mShape = AABB{.min = min, .max = max};
    }

    explicit ColliderComponent(const Model &model, const bool trigger = false, const float weight = 1.0f)
        : ColliderComponent(&model, trigger, weight) {}

    explicit ColliderComponent(const ColliderShape &shape, const bool trigger = false, const float weight = 1.0f)
        : mShape(shape), mTrigger(trigger), mWeight(weight) {}

    explicit ColliderComponent(const AABB &aabb, const bool trigger = false, const float weight = 1.0f)
        : mShape(aabb), mTrigger(trigger), mWeight(weight) {}

    explicit ColliderComponent(const Sphere &sphere, const bool trigger = false, const float weight = 1.0f)
        : mShape(sphere), mTrigger(trigger), mWeight(weight) {}

    explicit ColliderComponent(const Capsule &capsule, const bool trigger = false, const float weight = 1.0f)
        : mShape(capsule), mTrigger(trigger), mWeight(weight) {}

    explicit ColliderComponent(const Plane &plane, const bool trigger = false, const float weight = 1.0f)
        : mShape(plane), mTrigger(trigger), mWeight(weight) {}

    float getWeight() const { return mWeight; }
    void setWeight(const float weight) { mWeight = weight; }

    void setTrigger(const bool trigger) { mTrigger = trigger; }
    bool isTrigger() const { return mTrigger; }

    ColliderShape &getShape() { return mShape; }
    const ColliderShape &getShape() const { return mShape; }
    void setShape(const ColliderShape &shape) { mShape = shape; }

    bool isAABB() const { return std::holds_alternative<AABB>(mShape); }
    bool isSphere() const { return std::holds_alternative<Sphere>(mShape); }
    bool isCapsule() const { return std::holds_alternative<Capsule>(mShape); }
    bool isPlane() const { return std::holds_alternative<Plane>(mShape); }

    AABB &getAABB() { return std::get<AABB>(mShape); }
    const AABB &getAABB() const { return std::get<AABB>(mShape); }

    Sphere &getSphere() { return std::get<Sphere>(mShape); }
    const Sphere &getSphere() const { return std::get<Sphere>(mShape); }

    Capsule &getCapsule() { return std::get<Capsule>(mShape); }
    const Capsule &getCapsule() const { return std::get<Capsule>(mShape); }

    Plane &getPlane() { return std::get<Plane>(mShape); }
    const Plane &getPlane() const { return std::get<Plane>(mShape); }

    float getBoundingRadius() const
    {
        return std::visit([]<typename T0>(const T0 &shape) -> float
        {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, Sphere>) { return shape.radius + glm::length(shape.center); }
            else if constexpr (std::is_same_v<T, AABB>) { return glm::length(shape.max - shape.min) * 0.5f; }
            else if constexpr (std::is_same_v<T, Capsule>)
            {
                return shape.radius + glm::length(shape.b - shape.a) * 0.5f;
            }
            else { return 0.0f; }
        }, mShape);
    }

    std::function<void(Entity &self, Entity &other)> onCollision;

private:
    ColliderShape mShape{Sphere{}};
    bool mTrigger = false;
    float mWeight = 1.0f;
};
*/
#endif //OPENGL_COLLISION_COMPONENT_H
