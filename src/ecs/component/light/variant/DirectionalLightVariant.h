#ifndef OPENGL_DIRECTIONAL_LIGHT_VARIANT_H
#define OPENGL_DIRECTIONAL_LIGHT_VARIANT_H

struct DirectionalLightVariant : LightVariant
{
    glm::vec3 direction{0, -1, 0};

    Clonable *clone() const override
    {
        return new DirectionalLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override { codec.addVec3("Direction", &direction); }
};

REGISTER_VARIANT(LightVariantHolder, "Directional Light", DirectionalLightVariant);

#endif //OPENGL_DIRECTIONAL_LIGHT_VARIANT_H
