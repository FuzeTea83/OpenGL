#ifndef OPENGL_MESH_MANAGER_H
#define OPENGL_MESH_MANAGER_H

#include <filesystem>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "Mesh.h"
#include "utils/concurrency/CompletableFuture.h"

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_SortByPType | aiProcess_PopulateArmatureData)

#define ASSIMP_SUPPORTED_FORMATS {".3ds", ".ase", ".dae", ".fbx", ".glb", ".gltf", ".obj", ".ply", ".stl", ".x", ".lwo", ".lws", ".lxo", ".dxf", ".cob", ".scn", ".bvh"}

class Engine;
class MaterialManager;

class MeshManager
{
public:
    struct PreparedMesh
    {
        std::filesystem::path path;
        std::string name;
        bool generateMipmap{true};
        std::shared_ptr<Assimp::Importer> importer;
        const aiScene *scene{nullptr};
    };

    explicit MeshManager(Engine &engine, MaterialManager &materialManager);

    const Mesh &getMesh(MeshId);


    MeshDefinition loadMesh(const std::filesystem::path &path, const std::string_view &name,
                            bool generateMipmap = true, int assimpFlags = ASSIMP_LOAD_FLAGS);

    CompletableFuture<MeshDefinition> loadMeshAsync(const std::filesystem::path &path, const std::string_view &name,
                                                    bool generateMipmap = true, int assimpFlags = ASSIMP_LOAD_FLAGS);

    const std::vector<MeshDefinition> &getMeshDefinition();

private:
    void initFromScene(const aiScene *aiScene, const std::filesystem::path &path, Mesh &mesh,
                       bool generateMipmap) const;

    void initSubMesh(const aiScene *aiScene, const aiMesh *aiMesh, const std::filesystem::path &path,
                     const glm::mat4 &nodeTransform, const Skin *skin, SubMesh &subMesh,
                     bool generateMipmap) const;

    static void createBuffers(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices,
                              GLuint *vao, GLuint *vbo, GLuint *ibo);

    std::mutex mMutex;
    MaterialManager &mMaterialManager;
    std::vector<Mesh> mMeshes;
    std::unordered_map<std::size_t, MeshDefinition> mLookup;
    Engine &mEngine;
};
#endif //OPENGL_MESH_MANAGER_H
