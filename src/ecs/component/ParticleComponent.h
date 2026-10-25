#ifndef OPENGL_PARTICLE_COMPONENT_H
#define OPENGL_PARTICLE_COMPONENT_H

#include "ecs/core/Component.h"
#include "resource/texture/Texture.h"

struct ParticleComponent : Component
{
    float lifetime{1.0f};
    float size{1.0f};
    int count{1};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    TextureId texture{INVALID_TEXTURE_ID};
    glm::vec3 spread{0.0f};
    glm::vec3 velocity{0.0f, 1.0f, 0.0f};

    void initializeCodec(const Engine &engine) override
    {
        codec.addFloat("Lifetime", &lifetime);
        codec.addFloat("Size", &size);
        codec.addInt("Count", &count);
        codec.addColor4("Color", &color);
        //codec.addInt("Texture", &texture);
        codec.addVec3("Spread", &spread);
        codec.addVec3("Velocity", &velocity);
    }
};

#endif //OPENGL_PARTICLE_COMPONENT_H
