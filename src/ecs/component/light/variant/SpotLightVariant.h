#ifndef OPENGL_SPOT_LIGHT_VARIANT_H
#define OPENGL_SPOT_LIGHT_VARIANT_H

struct SpotLightVariant : LightVariant
{
    glm::vec3 direction{0, -1, 0};
    glm::vec3 attenuation{1, 0.09f, 0.032f};
    float innerCutoff{glm::cos(glm::radians(12.5f))};
    float outerCutoff{glm::cos(glm::radians(17.5f))};
    bool followRotation{true};
    
    Clonable *clone() const override
    {
        return new SpotLightVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Direction", &direction);
        codec.addVec3("Attenuation", &attenuation);
        codec.addFloat("Inner Cutoff", &innerCutoff);
        codec.addFloat("Outer Cutoff", &outerCutoff);
        codec.addBool("Follow Rotation", &followRotation);
    }
};

REGISTER_VARIANT(LightVariantHolder, "Spot Light", SpotLightVariant);

#endif //OPENGL_SPOT_LIGHT_VARIANT_H
