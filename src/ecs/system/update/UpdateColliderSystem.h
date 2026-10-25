#ifndef OPENGL_COLLISION_SYSTEM_H
#define OPENGL_COLLISION_SYSTEM_H
/*
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include "ecs/Entity.h"
#include "ecs/System.h"
#include "ecs/component/ColliderComponent.h"
#include "ecs/component/TransformComponent.h"

struct CollisionManifold
{
    bool hasCollision = false;
    glm::vec3 normal{0.0f, 1.0f, 0.0f}; // Direction outward from B to A (normal to push A out)
    float penetration = 0.0f;           // Penetration depth (> 0 when overlapping)
    glm::vec3 contactPoint{0.0f};
};

class UpdateColliderSystem : public UpdateSystem<TransformComponent, ColliderComponent>
{
public:
    explicit UpdateColliderSystem(const unsigned int priority) : UpdateSystem(priority) {}

    void onStart() override;

    void onUpdate(const float deltaTime, Entity &entity) override;

    void onStop() override;

    // Helpers to obtain world-space shapes
    static AABB getWorldAABB(const AABB &localAABB, const TransformComponent &transform);

    static Sphere getWorldSphere(const Sphere &localSphere, const TransformComponent &transform);

    static Capsule getWorldCapsule(const Capsule &localCapsule, const TransformComponent &transform);

    static Plane getWorldPlane(const Plane &localPlane, const TransformComponent &transform);

    static ColliderShape getWorldShape(const ColliderComponent &collider, const TransformComponent &transform);

    // Collision detection returning Manifolds
    static CollisionManifold checkShapeCollision(const ColliderShape &shapeA, const ColliderShape &shapeB);

    static CollisionManifold getCollisionManifold(const ColliderComponent &c1, const TransformComponent &t1,
                                                  const ColliderComponent &c2, const TransformComponent &t2);

    // Pairwise collision checks with manifold
    static CollisionManifold testAABBVsAABB(const AABB &a, const AABB &b);

    static CollisionManifold testSphereVsSphere(const Sphere &a, const Sphere &b);

    static CollisionManifold testCapsuleVsCapsule(const Capsule &a, const Capsule &b);

    static CollisionManifold testAABBvsSphere(const AABB &aabb, const Sphere &sphere);

    static CollisionManifold testSphereVsCapsule(const Sphere &sphere, const Capsule &capsule);

    static CollisionManifold testAABBvsCapsule(const AABB &aabb, const Capsule &capsule);

    static CollisionManifold testSphereVsPlane(const Sphere &sphere, const Plane &plane);

    static CollisionManifold testAABBVsPlane(const AABB &aabb, const Plane &plane);

    static CollisionManifold testCapsuleVsPlane(const Capsule &capsule, const Plane &plane);

    // Boolean checks (convenience)
    static bool checkCollision(const ColliderComponent &collider1, const ColliderComponent &collider2);

    static bool checkCollision(const ColliderComponent &c1, const TransformComponent &t1,
                               const ColliderComponent &c2, const TransformComponent &t2);

    static bool checkAABBVsAABB(const AABB &aabb1, const AABB &aabb2);

    static bool checkSphereVsSphere(const Sphere &sphere1, const Sphere &sphere2);

    static bool checkAABBvsSphere(const AABB &aabb, const Sphere &sphere);

    static bool checkCapsuleVsCapsule(const Capsule &capsule1, const Capsule &capsule2);

    static bool checkSphereVsCapsule(const Sphere &sphere, const Capsule &capsule);

    static bool checkAABBvsCapsule(const AABB &aabb, const Capsule &capsule);

    static bool checkSphereVsPlane(const Sphere &sphere, const Plane &plane);

    static bool checkAABBVsPlane(const AABB &aabb, const Plane &plane);

    static bool checkCapsuleVsPlane(const Capsule &capsule, const Plane &plane);

    // Math utilities
    static glm::vec3 closestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &p);

    static void closestPointsBetweenSegments(const glm::vec3 &p1, const glm::vec3 &q1,
                                             const glm::vec3 &p2, const glm::vec3 &q2,
                                             glm::vec3 &c1, glm::vec3 &c2);

    // Sweep test vs Plane (continuous collision detection)
    static float sweepVsPlane(const TransformComponent &transform, const ColliderComponent &collider,
                              const VelocityComponent &velocity, const Plane &ground);

    // Collision response resolution
    static void resolveCollision(Entity &entityA, Entity &entityB, const CollisionManifold &manifold);

private:
    std::vector<Entity *> mActiveEntities;
};
*/
#endif //OPENGL_COLLISION_SYSTEM_H
