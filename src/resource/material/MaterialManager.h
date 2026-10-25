#ifndef OPENGL_MATERIAL_MANAGER_H
#define OPENGL_MATERIAL_MANAGER_H

#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <assimp/material.h>
#include <assimp/scene.h>

#include "Material.h"

class TextureManager;

class MaterialManager
{
public:
    explicit MaterialManager(TextureManager &textureManager);

    const Material &getMaterial(MaterialId);

    MaterialId loadMaterial(const aiScene *scene, const aiMaterial *material, const std::filesystem::path &path,
                            bool generateMipmaps);

private:
    struct MaterialLoadContext
    {
        const aiScene *scene{nullptr};
        const aiMaterial *material{nullptr};
        std::filesystem::path directoryPath;
        std::filesystem::path filePath;
        Material &mat;
        bool generateMipmaps{false};
    };

    static float getValue(const aiMaterial *material, const char *key, unsigned int type, unsigned int index,
                          float defaultValue);

    static glm::vec3 getColor(const aiMaterial *material, const char *key, unsigned int type, unsigned int index,
                              glm::vec3 defaultColor);

    TextureId loadTexture(const MaterialLoadContext &context, aiTextureType type) const;

    static uint64_t hash(const Material &);

    std::mutex mMutex;
    TextureManager &mTextureManager;
    std::vector<Material> mMaterials;
    std::unordered_map<std::size_t, MaterialId> mLookup;
};
#endif //OPENGL_MATERIAL_MANAGER_H
