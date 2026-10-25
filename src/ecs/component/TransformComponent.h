#ifndef OPENGL_TRANSFORM_COMPONENT_H
#define OPENGL_TRANSFORM_COMPONENT_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ecs/core/Component.h"
#include "utils/Codec.h"

struct Transform : Clonable
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    Clonable *clone() const override
    {
        return new Transform(*this);
    }

    glm::vec3 getEulerAngles() const { return glm::eulerAngles(rotation); }

    glm::vec3 getRight() const { return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f)); }
    glm::vec3 getUp() const { return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f)); }
    glm::vec3 getForward() const { return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f)); }

    glm::vec3 getForwardXZ() const
    {
        glm::vec3 forward = getForward();
        forward.y = 0.0f;
        return glm::normalize(forward);
    }

    glm::vec3 getRightXZ() const
    {
        glm::vec3 right = getRight();
        right.y = 0.0f;
        return glm::normalize(right);
    }

    float getYaw() const { return glm::degrees(glm::eulerAngles(rotation).y); }
    float getPitch() const { return glm::degrees(glm::eulerAngles(rotation).x); }
    float getRoll() const { return glm::degrees(glm::eulerAngles(rotation).z); }

    glm::mat4 getModelMatrix() const
    {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, position);
        mat = mat * glm::mat4_cast(rotation);
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct TransformComponent : Component
{
    Transform current;
    Transform previous;

    Clonable *clone() const override
    {
        return new TransformComponent(*this);
    }

    glm::mat4 getModelMatrix(const float alpha) const
    {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, glm::mix(previous.position, current.position, alpha));
        mat = mat * glm::mat4_cast(glm::slerp(previous.rotation, current.rotation, alpha));
        mat = glm::scale(mat, glm::mix(previous.scale, current.scale, alpha));
        return mat;
    }

    void initializeCodec(const Engine &) override
    {
        codec.addVec3("Position", &current.position);
        codec.addQuaternion("Rotation", &current.rotation);
        codec.addVec3("Scale", &current.scale);
    }
};

#endif //OPENGL_TRANSFORM_COMPONENT_H
