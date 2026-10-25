/*#include "UpdateColliderSystem.h"
#include "ecs/ECS.h"

#include <cmath>
#include <limits>
#include <algorithm>

void UpdateColliderSystem::onStart() { mActiveEntities.clear(); }

void UpdateColliderSystem::onUpdate(const float, Entity &entity) { mActiveEntities.push_back(&entity); }

void UpdateColliderSystem::onStop()
{
    const std::size_t count = mActiveEntities.size();
    for (std::size_t i = 0; i < count; ++i)
    {
        Entity *entityA = mActiveEntities[i];
        if (entityA->isRemoved()) continue;

        auto &colliderA = entityA->getComponent<ColliderComponent>();
        auto &transformA = entityA->getComponent<TransformComponent>();
        ColliderShape worldShapeA = getWorldShape(colliderA, transformA);

        for (std::size_t j = i + 1; j < count; ++j)
        {
            Entity *entityB = mActiveEntities[j];
            if (entityB->isRemoved()) continue;

            auto &colliderB = entityB->getComponent<ColliderComponent>();
            auto &transformB = entityB->getComponent<TransformComponent>();
            ColliderShape worldShapeB = getWorldShape(colliderB, transformB);

            CollisionManifold manifold = checkShapeCollision(worldShapeA, worldShapeB);
            if (manifold.hasCollision)
            {
                if (colliderA.onCollision) colliderA.onCollision(*entityA, *entityB);
                if (colliderB.onCollision) colliderB.onCollision(*entityB, *entityA);

                resolveCollision(*entityA, *entityB, manifold);
            }
        }
    }
    mActiveEntities.clear();
}

AABB UpdateColliderSystem::getWorldAABB(const AABB &localAABB, const TransformComponent &transform)
{
    const glm::mat4 model = transform.getModelMatrix();
    glm::vec3 corners[8] = {
        {localAABB.min.x, localAABB.min.y, localAABB.min.z},
        {localAABB.max.x, localAABB.min.y, localAABB.min.z},
        {localAABB.min.x, localAABB.max.y, localAABB.min.z},
        {localAABB.max.x, localAABB.max.y, localAABB.min.z},
        {localAABB.min.x, localAABB.min.y, localAABB.max.z},
        {localAABB.max.x, localAABB.min.y, localAABB.max.z},
        {localAABB.min.x, localAABB.max.y, localAABB.max.z},
        {localAABB.max.x, localAABB.max.y, localAABB.max.z}
    };
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(-std::numeric_limits<float>::max());
    for (const auto &c: corners)
    {
        const auto wc = glm::vec3(model * glm::vec4(c, 1.0f));
        worldMin = glm::min(worldMin, wc);
        worldMax = glm::max(worldMax, wc);
    }
    return AABB{.min = worldMin, .max = worldMax};
}

Sphere UpdateColliderSystem::getWorldSphere(const Sphere &localSphere, const TransformComponent &transform)
{
    const glm::mat4 model = transform.getModelMatrix();
    const auto worldCenter = glm::vec3(model * glm::vec4(localSphere.center, 1.0f));
    const glm::vec3 scale = transform.getScale();
    const float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
    return Sphere{.center = worldCenter, .radius = localSphere.radius * maxScale};
}

Capsule UpdateColliderSystem::getWorldCapsule(const Capsule &localCapsule, const TransformComponent &transform)
{
    const glm::mat4 model = transform.getModelMatrix();
    const auto worldA = glm::vec3(model * glm::vec4(localCapsule.a, 1.0f));
    const auto worldB = glm::vec3(model * glm::vec4(localCapsule.b, 1.0f));
    const glm::vec3 scale = transform.getScale();
    const float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
    return Capsule{.a = worldA, .b = worldB, .radius = localCapsule.radius * maxScale};
}

Plane UpdateColliderSystem::getWorldPlane(const Plane &localPlane, const TransformComponent &transform)
{
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform.getModelMatrix())));
    const glm::vec3 worldNormal = glm::normalize(normalMatrix * localPlane.normal);
    const glm::vec3 localPoint = -localPlane.d * localPlane.normal;
    const auto worldPoint = glm::vec3(transform.getModelMatrix() * glm::vec4(localPoint, 1.0f));
    const float worldD = -glm::dot(worldNormal, worldPoint);
    return Plane{.normal = worldNormal, .d = worldD};
}

ColliderShape
UpdateColliderSystem::getWorldShape(const ColliderComponent &collider, const TransformComponent &transform)
{
    return std::visit([&transform]<typename T0>(const T0 &shape) -> ColliderShape
    {
        using T = std::decay_t<T0>;
        if constexpr (std::is_same_v<T, AABB>) return getWorldAABB(shape, transform);
        else if constexpr (std::is_same_v<T, Sphere>) return getWorldSphere(shape, transform);
        else if constexpr (std::is_same_v<T, Capsule>) return getWorldCapsule(shape, transform);
        else if constexpr (std::is_same_v<T, Plane>) return getWorldPlane(shape, transform);
        else throw std::runtime_error("Unknown collider shape type");
    }, collider.getShape());
}

CollisionManifold UpdateColliderSystem::checkShapeCollision(const ColliderShape &shapeA, const ColliderShape &shapeB)
{
    return std::visit([]<typename T0, typename T1>(const T0 &a, const T1 &b) -> CollisionManifold
    {
        using TA = std::decay_t<T0>;
        using TB = std::decay_t<T1>;

        if constexpr (std::is_same_v<TA, AABB> && std::is_same_v<TB, AABB>) { return testAABBVsAABB(a, b); }
        else if constexpr (std::is_same_v<TA, Sphere> && std::is_same_v<TB, Sphere>)
        {
            return testSphereVsSphere(a, b);
        }
        else if constexpr (std::is_same_v<TA, Capsule> && std::is_same_v<TB, Capsule>)
        {
            return testCapsuleVsCapsule(a, b);
        }
        else if constexpr (std::is_same_v<TA, AABB> && std::is_same_v<TB, Sphere>) { return testAABBvsSphere(a, b); }
        else if constexpr (std::is_same_v<TA, Sphere> && std::is_same_v<TB, AABB>)
        {
            CollisionManifold m = testAABBvsSphere(b, a);
            m.normal = -m.normal;
            return m;
        }
        else if constexpr (std::is_same_v<TA, Sphere> && std::is_same_v<TB, Capsule>)
        {
            return testSphereVsCapsule(a, b);
        }
        else if constexpr (std::is_same_v<TA, Capsule> && std::is_same_v<TB, Sphere>)
        {
            CollisionManifold m = testSphereVsCapsule(b, a);
            m.normal = -m.normal;
            return m;
        }
        else if constexpr (std::is_same_v<TA, AABB> && std::is_same_v<TB, Capsule>) { return testAABBvsCapsule(a, b); }
        else if constexpr (std::is_same_v<TA, Capsule> && std::is_same_v<TB, AABB>)
        {
            CollisionManifold m = testAABBvsCapsule(b, a);
            m.normal = -m.normal;
            return m;
        }
        else if constexpr (std::is_same_v<TA, Sphere> && std::is_same_v<TB, Plane>) { return testSphereVsPlane(a, b); }
        else if constexpr (std::is_same_v<TA, Plane> && std::is_same_v<TB, Sphere>)
        {
            CollisionManifold m = testSphereVsPlane(b, a);
            m.normal = -m.normal;
            return m;
        }
        else if constexpr (std::is_same_v<TA, AABB> && std::is_same_v<TB, Plane>) { return testAABBVsPlane(a, b); }
        else if constexpr (std::is_same_v<TA, Plane> && std::is_same_v<TB, AABB>)
        {
            CollisionManifold m = testAABBVsPlane(b, a);
            m.normal = -m.normal;
            return m;
        }
        else if constexpr (std::is_same_v<TA, Capsule> && std::is_same_v<TB, Plane>)
        {
            return testCapsuleVsPlane(a, b);
        }
        else if constexpr (std::is_same_v<TA, Plane> && std::is_same_v<TB, Capsule>)
        {
            CollisionManifold m = testCapsuleVsPlane(b, a);
            m.normal = -m.normal;
            return m;
        }
        else { return CollisionManifold{.hasCollision = false}; }
    }, shapeA, shapeB);
}

CollisionManifold UpdateColliderSystem::getCollisionManifold(const ColliderComponent &c1, const TransformComponent &t1,
                                                             const ColliderComponent &c2, const TransformComponent &t2)
{
    const ColliderShape w1 = getWorldShape(c1, t1);
    const ColliderShape w2 = getWorldShape(c2, t2);
    return checkShapeCollision(w1, w2);
}

CollisionManifold UpdateColliderSystem::testAABBVsAABB(const AABB &a, const AABB &b)
{
    CollisionManifold manifold;
    glm::vec3 minA = glm::min(a.min, a.max);
    glm::vec3 maxA = glm::max(a.min, a.max);
    glm::vec3 minB = glm::min(b.min, b.max);
    glm::vec3 maxB = glm::max(b.min, b.max);

    float overlapX = glm::min(maxA.x, maxB.x) - glm::max(minA.x, minB.x);
    float overlapY = glm::min(maxA.y, maxB.y) - glm::max(minA.y, minB.y);
    float overlapZ = glm::min(maxA.z, maxB.z) - glm::max(minA.z, minB.z);

    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    glm::vec3 centerA = (minA + maxA) * 0.5f;
    glm::vec3 centerB = (minB + maxB) * 0.5f;

    if (overlapX < overlapY && overlapX < overlapZ)
    {
        manifold.penetration = overlapX;
        manifold.normal = (centerA.x < centerB.x) ? glm::vec3(-1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else if (overlapY < overlapZ)
    {
        manifold.penetration = overlapY;
        manifold.normal = (centerA.y < centerB.y) ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        manifold.penetration = overlapZ;
        manifold.normal = (centerA.z < centerB.z) ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
    }

    manifold.contactPoint = (centerA + centerB) * 0.5f;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testSphereVsSphere(const Sphere &a, const Sphere &b)
{
    CollisionManifold manifold;
    const glm::vec3 delta = a.center - b.center;
    const float distSq = glm::dot(delta, delta);
    const float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    const float dist = std::sqrt(distSq);
    if (dist > 1e-5f)
    {
        manifold.normal = delta / dist;
        manifold.penetration = radiusSum - dist;
    }
    else
    {
        manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        manifold.penetration = radiusSum;
    }
    manifold.contactPoint = b.center + manifold.normal * (b.radius - manifold.penetration * 0.5f);
    return manifold;
}

CollisionManifold UpdateColliderSystem::testCapsuleVsCapsule(const Capsule &a, const Capsule &b)
{
    CollisionManifold manifold;
    glm::vec3 c1, c2;
    closestPointsBetweenSegments(a.a, a.b, b.a, b.b, c1, c2);

    const glm::vec3 delta = c1 - c2;
    const float distSq = glm::dot(delta, delta);
    const float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    const float dist = std::sqrt(distSq);
    if (dist > 1e-5f)
    {
        manifold.normal = delta / dist;
        manifold.penetration = radiusSum - dist;
    }
    else
    {
        manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        manifold.penetration = radiusSum;
    }
    manifold.contactPoint = c2 + manifold.normal * (b.radius - manifold.penetration * 0.5f);
    return manifold;
}

CollisionManifold UpdateColliderSystem::testAABBvsSphere(const AABB &aabb, const Sphere &sphere)
{
    CollisionManifold manifold;
    glm::vec3 minA = glm::min(aabb.min, aabb.max);
    glm::vec3 maxA = glm::max(aabb.min, aabb.max);

    glm::vec3 closestPoint(
        glm::clamp(sphere.center.x, minA.x, maxA.x),
        glm::clamp(sphere.center.y, minA.y, maxA.y),
        glm::clamp(sphere.center.z, minA.z, maxA.z)
    );

    glm::vec3 delta = closestPoint - sphere.center;
    float distSq = glm::dot(delta, delta);

    if (distSq > sphere.radius * sphere.radius)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    float dist = std::sqrt(distSq);

    if (dist > 1e-5f)
    {
        manifold.normal = delta / dist;
        manifold.penetration = sphere.radius - dist;
    }
    else
    {
        float dMinX = sphere.center.x - minA.x;
        float dMaxX = maxA.x - sphere.center.x;
        float dMinY = sphere.center.y - minA.y;
        float dMaxY = maxA.y - sphere.center.y;
        float dMinZ = sphere.center.z - minA.z;
        float dMaxZ = maxA.z - sphere.center.z;

        float minFaceDist = dMinX;
        manifold.normal = glm::vec3(-1.0f, 0.0f, 0.0f);

        if (dMaxX < minFaceDist)
        {
            minFaceDist = dMaxX;
            manifold.normal = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        if (dMinY < minFaceDist)
        {
            minFaceDist = dMinY;
            manifold.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        if (dMaxY < minFaceDist)
        {
            minFaceDist = dMaxY;
            manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (dMinZ < minFaceDist)
        {
            minFaceDist = dMinZ;
            manifold.normal = glm::vec3(0.0f, 0.0f, -1.0f);
        }
        if (dMaxZ < minFaceDist)
        {
            minFaceDist = dMaxZ;
            manifold.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        manifold.penetration = sphere.radius + minFaceDist;
    }
    manifold.contactPoint = closestPoint;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testSphereVsCapsule(const Sphere &sphere, const Capsule &capsule)
{
    CollisionManifold manifold;
    const glm::vec3 closestPoint = closestPointOnSegment(capsule.a, capsule.b, sphere.center);
    const glm::vec3 delta = sphere.center - closestPoint;
    const float distSq = glm::dot(delta, delta);
    const float radiusSum = sphere.radius + capsule.radius;

    if (distSq > radiusSum * radiusSum)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    const float dist = std::sqrt(distSq);
    if (dist > 1e-5f)
    {
        manifold.normal = delta / dist;
        manifold.penetration = radiusSum - dist;
    }
    else
    {
        manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        manifold.penetration = radiusSum;
    }
    manifold.contactPoint = closestPoint + manifold.normal * capsule.radius;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testAABBvsCapsule(const AABB &aabb, const Capsule &capsule)
{
    CollisionManifold manifold;
    glm::vec3 minA = glm::min(aabb.min, aabb.max);
    glm::vec3 maxA = glm::max(aabb.min, aabb.max);

    static constexpr int SAMPLES = 16;
    float minDistSq = std::numeric_limits<float>::max();
    glm::vec3 bestSample(0.0f);
    glm::vec3 bestClosestAABB(0.0f);

    for (int i = 0; i <= SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(SAMPLES);
        glm::vec3 sample = glm::mix(capsule.a, capsule.b, t);
        glm::vec3 clamped(
            glm::clamp(sample.x, minA.x, maxA.x),
            glm::clamp(sample.y, minA.y, maxA.y),
            glm::clamp(sample.z, minA.z, maxA.z)
        );
        glm::vec3 delta = sample - clamped;
        float dSq = glm::dot(delta, delta);
        if (dSq < minDistSq)
        {
            minDistSq = dSq;
            bestSample = sample;
            bestClosestAABB = clamped;
        }
    }

    if (minDistSq > capsule.radius * capsule.radius)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    float dist = std::sqrt(minDistSq);
    glm::vec3 delta = bestClosestAABB - bestSample;
    if (dist > 1e-5f)
    {
        manifold.normal = delta / dist;
        manifold.penetration = capsule.radius - dist;
    }
    else
    {
        manifold.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        manifold.penetration = capsule.radius;
    }
    manifold.contactPoint = bestClosestAABB;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testSphereVsPlane(const Sphere &sphere, const Plane &plane)
{
    CollisionManifold manifold;
    const glm::vec3 n = glm::normalize(plane.normal);
    const float distToPlane = glm::dot(n, sphere.center) + plane.d;

    if (distToPlane > sphere.radius)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    manifold.normal = n;
    manifold.penetration = sphere.radius - distToPlane;
    manifold.contactPoint = sphere.center - n * distToPlane;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testAABBVsPlane(const AABB &aabb, const Plane &plane)
{
    CollisionManifold manifold;
    glm::vec3 n = glm::normalize(plane.normal);
    glm::vec3 minA = glm::min(aabb.min, aabb.max);
    glm::vec3 maxA = glm::max(aabb.min, aabb.max);

    glm::vec3 center = (minA + maxA) * 0.5f;
    glm::vec3 extents = (maxA - minA) * 0.5f;

    float r = extents.x * std::abs(n.x) + extents.y * std::abs(n.y) + extents.z * std::abs(n.z);
    float distToPlane = glm::dot(n, center) + plane.d;

    if (distToPlane > r)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    manifold.normal = n;
    manifold.penetration = r - distToPlane;
    manifold.contactPoint = center - n * distToPlane;
    return manifold;
}

CollisionManifold UpdateColliderSystem::testCapsuleVsPlane(const Capsule &capsule, const Plane &plane)
{
    CollisionManifold manifold;
    const glm::vec3 n = glm::normalize(plane.normal);
    const float distA = glm::dot(n, capsule.a) + plane.d;
    const float distB = glm::dot(n, capsule.b) + plane.d;
    const float minDist = glm::min(distA, distB);

    if (minDist > capsule.radius)
    {
        manifold.hasCollision = false;
        return manifold;
    }

    manifold.hasCollision = true;
    manifold.normal = n;
    manifold.penetration = capsule.radius - minDist;
    manifold.contactPoint = (distA < distB ? capsule.a : capsule.b) - n * minDist;
    return manifold;
}

bool UpdateColliderSystem::checkCollision(const ColliderComponent &collider1, const ColliderComponent &collider2)
{
    return checkShapeCollision(collider1.getShape(), collider2.getShape()).hasCollision;
}

bool UpdateColliderSystem::checkCollision(const ColliderComponent &c1, const TransformComponent &t1,
                                          const ColliderComponent &c2, const TransformComponent &t2)
{
    return getCollisionManifold(c1, t1, c2, t2).hasCollision;
}

bool UpdateColliderSystem::checkAABBVsAABB(const AABB &aabb1, const AABB &aabb2)
{
    return testAABBVsAABB(aabb1, aabb2).hasCollision;
}

bool UpdateColliderSystem::checkSphereVsSphere(const Sphere &sphere1, const Sphere &sphere2)
{
    return testSphereVsSphere(sphere1, sphere2).hasCollision;
}

bool UpdateColliderSystem::checkAABBvsSphere(const AABB &aabb, const Sphere &sphere)
{
    return testAABBvsSphere(aabb, sphere).hasCollision;
}

bool UpdateColliderSystem::checkCapsuleVsCapsule(const Capsule &capsule1, const Capsule &capsule2)
{
    return testCapsuleVsCapsule(capsule1, capsule2).hasCollision;
}

bool UpdateColliderSystem::checkSphereVsCapsule(const Sphere &sphere, const Capsule &capsule)
{
    return testSphereVsCapsule(sphere, capsule).hasCollision;
}

bool UpdateColliderSystem::checkAABBvsCapsule(const AABB &aabb, const Capsule &capsule)
{
    return testAABBvsCapsule(aabb, capsule).hasCollision;
}

bool UpdateColliderSystem::checkSphereVsPlane(const Sphere &sphere, const Plane &plane)
{
    return testSphereVsPlane(sphere, plane).hasCollision;
}

bool UpdateColliderSystem::checkAABBVsPlane(const AABB &aabb, const Plane &plane)
{
    return testAABBVsPlane(aabb, plane).hasCollision;
}

bool UpdateColliderSystem::checkCapsuleVsPlane(const Capsule &capsule, const Plane &plane)
{
    return testCapsuleVsPlane(capsule, plane).hasCollision;
}

glm::vec3 UpdateColliderSystem::closestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &p)
{
    const glm::vec3 ab = b - a;
    const float lenSq = glm::dot(ab, ab);
    if (lenSq < 1e-6f) return a;
    float t = glm::dot(p - a, ab) / lenSq;
    t = glm::clamp(t, 0.0f, 1.0f);
    return a + t * ab;
}

void UpdateColliderSystem::closestPointsBetweenSegments(
    const glm::vec3 &p1, const glm::vec3 &q1,
    const glm::vec3 &p2, const glm::vec3 &q2,
    glm::vec3 &c1, glm::vec3 &c2)
{
    const glm::vec3 d1 = q1 - p1;
    const glm::vec3 d2 = q2 - p2;
    const glm::vec3 r = p1 - p2;

    const float a = glm::dot(d1, d1);
    const float e = glm::dot(d2, d2);
    const float f = glm::dot(d2, r);

    float s = 0.0f;
    float t = 0.0f;

    if (a <= 1e-6f && e <= 1e-6f)
    {
        c1 = p1;
        c2 = p2;
        return;
    }

    if (a <= 1e-6f)
    {
        s = 0.0f;
        t = glm::clamp(f / e, 0.0f, 1.0f);
    }
    else
    {
        float c = glm::dot(d1, r);
        if (e <= 1e-6f)
        {
            t = 0.0f;
            s = glm::clamp(-c / a, 0.0f, 1.0f);
        }
        else
        {
            float b = glm::dot(d1, d2);
            float denom = a * e - b * b;

            if (denom > 1e-6f) { s = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f); }
            else { s = 0.0f; }

            t = (b * s + f) / e;

            if (t < 0.0f)
            {
                t = 0.0f;
                s = glm::clamp(-c / a, 0.0f, 1.0f);
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = glm::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }

    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
}

float UpdateColliderSystem::sweepVsPlane(const TransformComponent &transform, const ColliderComponent &collider,
                                         const VelocityComponent &velocity, const Plane &ground)
{
    const glm::vec3 n = glm::normalize(ground.normal);
    const float radius = collider.getBoundingRadius();
    const float distToPlane = glm::dot(n, transform.getPosition()) + ground.d - radius;

    const float velAlongNormal = glm::dot(n, velocity.getVelocity());
    if (velAlongNormal >= -1e-5f) return 1.0f;

    if (distToPlane <= 0.0f) return 0.0f;

    const float toi = -distToPlane / velAlongNormal;
    return glm::clamp(toi, 0.0f, 1.0f);
}

void UpdateColliderSystem::resolveCollision(Entity &entityA, Entity &entityB, const CollisionManifold &manifold)
{
    auto &transformA = entityA.getComponent<TransformComponent>();
    auto &colliderA = entityA.getComponent<ColliderComponent>();
    auto &transformB = entityB.getComponent<TransformComponent>();
    auto &colliderB = entityB.getComponent<ColliderComponent>();

    if (colliderA.isTrigger() || colliderB.isTrigger()) { return; }

    bool hasVelA = entityA.hasComponent<VelocityComponent>();
    bool hasVelB = entityB.hasComponent<VelocityComponent>();

    float weightA = colliderA.getWeight();
    float weightB = colliderB.getWeight();

    float invMassA = (hasVelA && weightA > 0.0f) ? (1.0f / weightA) : 0.0f;
    float invMassB = (hasVelB && weightB > 0.0f) ? (1.0f / weightB) : 0.0f;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.0001f) { return; }

    const float penetration = glm::max(manifold.penetration - 0.001f, 0.0f);
    const glm::vec3 moveA = manifold.normal * (penetration * (invMassA / totalInvMass));
    const glm::vec3 moveB = -manifold.normal * (penetration * (invMassB / totalInvMass));

    transformA.translate(moveA);
    transformB.translate(moveB);

    glm::vec3 velA = hasVelA ? entityA.getComponent<VelocityComponent>().getVelocity() : glm::vec3(0.0f);
    glm::vec3 velB = hasVelB ? entityB.getComponent<VelocityComponent>().getVelocity() : glm::vec3(0.0f);

    glm::vec3 relVel = velA - velB;
    float velAlongNormal = glm::dot(relVel, manifold.normal);

    if (velAlongNormal < 0.0f)
    {
        float restitution = 0.2f;
        float j = -(1.0f + restitution) * velAlongNormal / totalInvMass;
        glm::vec3 impulse = manifold.normal * j;

        if (hasVelA && invMassA > 0.0f)
        {
            auto &vCompA = entityA.getComponent<VelocityComponent>();
            vCompA.setVelocity(velA + impulse * invMassA);
        }
        if (hasVelB && invMassB > 0.0f)
        {
            auto &vCompB = entityB.getComponent<VelocityComponent>();
            vCompB.setVelocity(velB - impulse * invMassB);
        }
    }
}*/
