#ifndef OPENGL_LIGHT_COMPONENT_H
#define OPENGL_LIGHT_COMPONENT_H

#include <glm/vec3.hpp>
#include "ecs/core/Component.h"

struct LightVariant : ComponentVariant {};

struct LightVariantHolder : VariantHolder<LightVariant> {};

struct LightComponent : Component
{
    glm::vec3 offset{0.0f};
    glm::vec3 color{1.0f};
    float ambient{0.1f};
    float diffuse{1.0f};
    float specular{1.0f};
    float intensity{1.0f};
    LightVariantHolder variant;

    Clonable *clone() const override
    {
        const auto component = new LightComponent();
        component->offset = offset;
        component->color = color;
        component->ambient = ambient;
        component->diffuse = diffuse;
        component->specular = specular;
        component->intensity = intensity;
        if (variant.get() != nullptr)
        {
            component->variant.adopt(static_cast<LightVariant *>(variant.clone()), variant.getVariantName());
        }
        return component;
    }

    void initializeCodec(const Engine &engine) override
    {
        codec.addVec3("Offset", &offset);
        codec.addColor3("Color", &color);
        codec.addFloat("Ambient", &ambient);
        codec.addFloat("Diffuse", &diffuse);
        codec.addFloat("Specular", &specular);
        codec.addFloat("Intensity", &intensity);
        codec.addOption("Variant", new Codec::Option{
                            .selectedOption = variant.getVariantName(),
                            .options = variant.getVariants(),
                            .onSelected = [this](const std::string &name)
                            {
                                variant.setVariant(name);
                                codec.markDirty();
                            }
                        });
        variant.initializeCodec(engine, codec);
    }

    bool hasVariant() const override
    {
        return true;
    }

    void setVariant(const std::string &name) override
    {
        variant.setVariant(name);
    }
};

#endif //OPENGL_LIGHT_COMPONENT_H
