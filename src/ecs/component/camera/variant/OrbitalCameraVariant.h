#ifndef OPENGL_ORBITAL_CAMERA_VARIANT_H
#define OPENGL_ORBITAL_CAMERA_VARIANT_H

#include "ecs/component/camera/CameraComponent.h"

struct OrbitalCameraVariant : CameraVariant
{
    glm::vec3 offset{0.0f};
    float distance{5.0f};

    Clonable *clone() const override
    {
        return new OrbitalCameraVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Offset", &offset);
        codec.addFloat("Distance", &distance);
    }
};

REGISTER_VARIANT(CameraVariantHolder, "Orbital Camera", OrbitalCameraVariant);

#endif //OPENGL_ORBITAL_CAMERA_VARIANT_H
