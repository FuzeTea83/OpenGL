#include "MaterialManager.h"
#include "resource/texture/TextureManager.h"

#include <fstream>
#include <iostream>
#include <assimp/GltfMaterial.h>
#include <assimp/scene.h>

MaterialManager::MaterialManager(TextureManager &textureManager) : mTextureManager(textureManager) {}

const Material &MaterialManager::getMaterial(const MaterialId id)
{
    std::lock_guard lock(mMutex);
    if (id > mMaterials.size() || id <= 0) throw std::runtime_error("Invalid material ID");
    return mMaterials[id - 1];
}

MaterialId MaterialManager::loadMaterial(const aiScene *scene, const aiMaterial *material,
                                         const std::filesystem::path &path, const bool generateMipmaps)
{
    Material mat;
    MaterialLoadContext context{
        .scene = scene,
        .material = material,
        .directoryPath = path.parent_path(),
        .filePath = path,
        .mat = mat,
        .generateMipmaps = generateMipmaps
    };

    aiColor4D baseColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
    {
        mat.diffuseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
    }
    else { mat.diffuseColor = getColor(material, AI_MATKEY_COLOR_DIFFUSE, glm::vec3(1.0f)); }

    mat.specularColor = getColor(material, AI_MATKEY_COLOR_SPECULAR, glm::vec3(1.0f));
    mat.ambientColor = getColor(material, AI_MATKEY_COLOR_AMBIENT, mat.diffuseColor);
    mat.emissiveColor = getColor(material, AI_MATKEY_COLOR_EMISSIVE, glm::vec3(0.0f));
    mat.shininess = getValue(material, AI_MATKEY_SHININESS, 32.0f);

    mat.metallic = getValue(material, AI_MATKEY_METALLIC_FACTOR, 0.0f);
    mat.roughness = getValue(material, AI_MATKEY_ROUGHNESS_FACTOR, 0.5f);
    mat.intensity = getValue(material, AI_MATKEY_EMISSIVE_INTENSITY, 0.0f);
    mat.alphaCutoff = getValue(material, AI_MATKEY_GLTF_ALPHACUTOFF, 0.5f);

    mat.diffuseMap = loadTexture(context, aiTextureType_DIFFUSE);
    mat.specularMap = loadTexture(context, aiTextureType_SPECULAR);
    mat.normalMap = loadTexture(context, aiTextureType_NORMALS);
    mat.metallicRoughnessMap = loadTexture(context, aiTextureType_DIFFUSE_ROUGHNESS);
    mat.emissiveMap = loadTexture(context, aiTextureType_EMISSIVE);
    mat.aoMap = loadTexture(context, aiTextureType_AMBIENT_OCCLUSION);

    if (mat.metallic != 0 || mat.roughness != 0.5f || mat.metallicRoughnessMap != INVALID_TEXTURE_ID)
    {
        mat.ambientColor = glm::vec3(0);
        mat.isPBR = true;
    }

    if (mat.diffuseMap != INVALID_TEXTURE_ID && mat.diffuseColor == glm::vec3(0.0f))
    {
        mat.diffuseColor = glm::vec3(1.0f);
    }

    aiString alphaMode;
    if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
    {
        mat.transparent = std::string_view(alphaMode.C_Str()) == "BLEND";
        mat.alphaMasked = std::string_view(alphaMode.C_Str()) == "MASK";
    }

    std::size_t h = hash(mat);

    std::lock_guard lock(mMutex);
    const auto it = mLookup.find(h);
    if (it != mLookup.end()) return it->second;

    const MaterialId id = static_cast<MaterialId>(mMaterials.size()) + 1;

    mMaterials.push_back(mat);
    mLookup.emplace(h, id);
    return id;
}

float MaterialManager::getValue(const aiMaterial *material, const char *key, const unsigned int type,
                                const unsigned int index, const float defaultValue)
{
    float value = defaultValue;
    if (material->Get(key, type, index, value) == AI_SUCCESS) { return value; }
    return defaultValue;
}

glm::vec3 MaterialManager::getColor(const aiMaterial *material, const char *key, const unsigned int type,
                                    const unsigned int index, const glm::vec3 defaultColor)
{
    aiColor4D color4(0.f, 0.f, 0.f, 1.f);
    if (material->Get(key, type, index, color4) == AI_SUCCESS) { return {color4.r, color4.g, color4.b}; }
    aiColor3D color(0.f, 0.f, 0.f);
    if (material->Get(key, type, index, color) == AI_SUCCESS) { return {color.r, color.g, color.b}; }
    return defaultColor;
}

TextureId MaterialManager::loadTexture(const MaterialLoadContext &context, const aiTextureType type) const
{
    if (context.material->GetTextureCount(type) <= 0)
    {
        if (type == aiTextureType_DIFFUSE && context.material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
        {
            return loadTexture(context, aiTextureType_BASE_COLOR);
        }
        if (type == aiTextureType_NORMALS)
        {
            if (context.material->GetTextureCount(aiTextureType_HEIGHT) > 0)
            {
                return loadTexture(context, aiTextureType_HEIGHT);
            }
            if (context.material->GetTextureCount(aiTextureType_NORMAL_CAMERA) > 0)
            {
                return loadTexture(context, aiTextureType_NORMAL_CAMERA);
            }
        }
        if (type == aiTextureType_DIFFUSE_ROUGHNESS)
        {
            if (context.material->GetTextureCount(aiTextureType_METALNESS) > 0)
            {
                return loadTexture(context, aiTextureType_METALNESS);
            }
            if (context.material->GetTextureCount(aiTextureType_UNKNOWN) > 0)
            {
                return loadTexture(context, aiTextureType_UNKNOWN);
            }
        }
        if (type == aiTextureType_AMBIENT_OCCLUSION)
        {
            if (context.material->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
            {
                return loadTexture(context, aiTextureType_LIGHTMAP);
            }
            if (context.material->GetTextureCount(aiTextureType_AMBIENT) > 0)
            {
                return loadTexture(context, aiTextureType_AMBIENT);
            }
        }
        return INVALID_TEXTURE_ID;
    }

    aiString path;

    if (context.material->GetTexture(type, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) !=
        AI_SUCCESS) { return INVALID_TEXTURE_ID; }

    if (const aiTexture *aiTexture = context.scene->GetEmbeddedTexture(path.C_Str()))
    {
        if (aiTexture->mHeight == 0)
        {
            return mTextureManager.loadTextureFromMemoryAsync(aiTexture->pcData, aiTexture->mWidth,
                                                              context.generateMipmaps,
                                                              type == aiTextureType_EMISSIVE).get();
        }
        return mTextureManager.loadTextureFromMemoryAsync(
            aiTexture->pcData,
            aiTexture->mWidth * aiTexture->mHeight * sizeof(aiTexel),
            context.generateMipmaps, type == aiTextureType_EMISSIVE).get();
    }
    return mTextureManager.loadTextureAsync(context.directoryPath / path.C_Str(), context.generateMipmaps,
                                            type == aiTextureType_EMISSIVE).get();
}

uint64_t MaterialManager::hash(const Material &material)
{
    uint64_t h = 0xdeadbeefcafeULL;
    auto combine = [&](const uint64_t v) { h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };

    auto hashFloat = [](const float f)
    {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));
        if (bits == 0x80000000u) bits = 0;
        return std::hash<uint32_t>{}(bits);
    };
    auto hashVec3 = [&](const glm::vec3 &v)
    {
        combine(hashFloat(v.x));
        combine(hashFloat(v.y));
        combine(hashFloat(v.z));
    };

    auto hashInt = [](const unsigned int i) { return std::hash<unsigned int>{}(i); };

    hashVec3(material.ambientColor);
    hashVec3(material.diffuseColor);
    hashVec3(material.specularColor);
    hashVec3(material.emissiveColor);
    combine(hashFloat(material.roughness));
    combine(hashFloat(material.metallic));
    combine(hashFloat(material.ao));
    combine(hashFloat(material.intensity));
    combine(hashFloat(material.shininess));
    combine(hashInt(material.diffuseMap));
    combine(hashInt(material.specularMap));
    combine(hashInt(material.normalMap));
    combine(hashInt(material.metallicRoughnessMap));
    combine(hashInt(material.emissiveMap));
    combine(hashInt(material.aoMap));
    combine(std::hash<bool>{}(material.transparent));
    combine(std::hash<bool>{}(material.alphaMasked));
    combine(hashFloat(material.alphaCutoff));

    return h;
}
