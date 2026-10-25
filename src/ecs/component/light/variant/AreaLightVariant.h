#ifndef OPENGL_AREA_LIGHT_VARIANT_H
#define OPENGL_AREA_LIGHT_VARIANT_H

#include "ecs/component/light/LightComponent.h"

struct AreaLightVariant : LightVariant
{
    glm::vec3 direction{0, -1, 0};
    glm::vec3 attenuation{1, 0.09f, 0.032f};
    float width = 1;
    float height = 1;

    Clonable *clone() const override
    {
        return new AreaLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Direction", &direction);
        codec.addVec3("Attenuation", &attenuation);
        codec.addFloat("Width", &width);
        codec.addFloat("Height", &height);
    }
};

REGISTER_VARIANT(LightVariantHolder, "Area Light", AreaLightVariant);

#endif //OPENGL_AREA_LIGHT_VARIANT_H
