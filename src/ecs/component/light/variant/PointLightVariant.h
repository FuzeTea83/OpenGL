#ifndef OPENGL_POINT_LIGHT_VARIANT_H
#define OPENGL_POINT_LIGHT_VARIANT_H

struct PointLightVariant : LightVariant
{
    glm::vec3 attenuation{1, 0.09f, 0.032f};

    Clonable *clone() const override
    {
        return new PointLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override { codec.addVec3("Attenuation", &attenuation); }
};

REGISTER_VARIANT(LightVariantHolder, "Point Light", PointLightVariant);

#endif //OPENGL_POINT_LIGHT_VARIANT_H
