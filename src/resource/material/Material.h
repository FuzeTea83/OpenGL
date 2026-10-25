#ifndef OPENGL_MATERIAL_H
#define OPENGL_MATERIAL_H

#include <glm/vec3.hpp>
#include "resource/texture/Texture.h"

using MaterialId = unsigned int;
constexpr MaterialId INVALID_MATERIAL_ID = 0;

struct Material
{
    glm::vec3 ambientColor{0.1f};
    glm::vec3 diffuseColor{1.0f};
    glm::vec3 specularColor{1.0f};
    glm::vec3 emissiveColor{0.0f};

    float roughness{0.5f};
    float metallic{0.0f};
    float ao{1.0f};
    float intensity{0.0f};
    float shininess{32.0f};

    TextureId diffuseMap{INVALID_TEXTURE_ID};
    TextureId specularMap{INVALID_TEXTURE_ID};
    TextureId normalMap{INVALID_TEXTURE_ID};
    TextureId metallicRoughnessMap{INVALID_TEXTURE_ID};
    TextureId emissiveMap{INVALID_TEXTURE_ID};
    TextureId aoMap{INVALID_TEXTURE_ID};

    bool isPBR{false};
    bool transparent{false};
    bool alphaMasked{false};
    float alphaCutoff{0.5f};
};
#endif //OPENGL_MATERIAL_H
