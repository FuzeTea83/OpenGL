#ifndef OPENGL_SPHERE_LIGHT_VARIANT_H
#define OPENGL_SPHERE_LIGHT_VARIANT_H

#include "ecs/component/light/LightComponent.h"

struct SphereLightVariant : LightVariant
{
    glm::vec3 attenuation{1, 0.09f, 0.032f};
    float radius = 1;

    Clonable *clone() const override
    {
        return new SphereLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Attenuation", &attenuation);
        codec.addFloat("Radius", &radius);
    }
};

REGISTER_VARIANT(LightVariantHolder, "Sphere Light", SphereLightVariant);

#endif //OPENGL_SPHERE_LIGHT_VARIANT_H
