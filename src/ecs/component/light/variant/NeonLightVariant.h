#ifndef OPENGL_NEON_LIGHT_VARIANT_H
#define OPENGL_NEON_LIGHT_VARIANT_H

struct NeonLightVariant : LightVariant
{
    glm::vec3 direction{0, -1, 0};
    glm::vec3 attenuation{1, 0.09f, 0.032f};
    float radius{1.0f};

    Clonable *clone() const override
    {
        return new NeonLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Direction", &direction);
        codec.addVec3("Attenuation", &attenuation);
        codec.addFloat("Radius", &radius);
    }
};

REGISTER_VARIANT(LightVariantHolder, "Neon Light", NeonLightVariant);

#endif //OPENGL_NEON_LIGHT_VARIANT_H
