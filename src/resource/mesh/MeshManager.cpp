#include "MeshManager.h"

#include <cstddef>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/Engine.h"
#include "resource/material/MaterialManager.h"

namespace
{
glm::mat4 toGlm(const aiMatrix4x4 &matrix)
{
    return {
        matrix.a1, matrix.b1, matrix.c1, matrix.d1,
        matrix.a2, matrix.b2, matrix.c2, matrix.d2,
        matrix.a3, matrix.b3, matrix.c3, matrix.d3,
        matrix.a4, matrix.b4, matrix.c4, matrix.d4
    };
}

void assignNodeTransforms(const aiNode *node, const aiMatrix4x4 &parentTransform,
                          std::vector<glm::mat4> &meshTransforms)
{
    const aiMatrix4x4 globalTransform = parentTransform * node->mTransformation;
    const glm::mat4 transform = toGlm(globalTransform);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) { meshTransforms[node->mMeshes[i]] = transform; }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        assignNodeTransforms(node->mChildren[i], globalTransform, meshTransforms);
    }
}
}

MeshManager::MeshManager(Engine &engine, MaterialManager &materialManager)
    : mMaterialManager(materialManager), mEngine(engine) {}

const Mesh &MeshManager::getMesh(const MeshId id)
{
    std::lock_guard lock(mMutex);
    if (id > mMeshes.size() || id <= 0) throw std::runtime_error("Invalid mesh ID");
    return mMeshes[id - 1];
}


MeshDefinition MeshManager::loadMesh(const std::filesystem::path &path, const std::string_view &name,
                                     const bool generateMipmap, const int assimpFlags)
{
    std::size_t hash = std::hash<std::filesystem::path>{}(path);
    {
        std::lock_guard lock(mMutex);
        const auto it = mLookup.find(hash);
        if (it != mLookup.end()) { return it->second; }
    }

    std::cout << "Loading mesh: " << path << std::endl;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path.string().c_str(), assimpFlags);

    MeshDefinition definition{
        .name = std::string(name), .path = path, .id = INVALID_MESH_ID, .generateMipmap = generateMipmap
    };


    if (scene == nullptr)
    {
        std::cerr << "Error parsing '" << path << "': '" << importer.GetErrorString() << "'" << std::endl;
        return definition;
    }


    Mesh mesh;
    initFromScene(scene, path, mesh, generateMipmap);

    std::lock_guard lock(mMutex);
    const MeshId id = static_cast<uint32_t>(mMeshes.size()) + 1;
    definition.id = id;
    MeshDefinition copy = definition;
    mLookup.emplace(hash, std::move(copy));
    mMeshes.push_back(std::move(mesh));
    return definition;
}


CompletableFuture<MeshDefinition> MeshManager::loadMeshAsync(const std::filesystem::path &path,
                                                             const std::string_view &name, const bool generateMipmap,
                                                             const int assimpFlags)
{
    return CompletableFuture<MeshDefinition>::runAsync(
        [this, path = std::filesystem::absolute(path), name = std::string(name), generateMipmap, assimpFlags]
        {
            return loadMesh(path, name, generateMipmap, assimpFlags);
        }, mEngine.getEngineExecutor());
}

const std::vector<MeshDefinition> &MeshManager::getMeshDefinition()
{
    static std::vector<MeshDefinition> meshDefinitions;
    std::lock_guard lock(mMutex);
    meshDefinitions.clear();
    for (const auto &def: mLookup | std::views::values) { meshDefinitions.push_back({.name = def.name, .id = def.id}); }
    return meshDefinitions;
}

static void buildNodes(const aiNode *source, const int parent, Mesh &mesh,
                       std::unordered_map<const aiNode *, int> &nodeMap)
{
    const int index = static_cast<int>(mesh.nodes.size());
    nodeMap[source] = index;
    Node node;
    node.name = source->mName.C_Str();
    node.parent = parent;
    node.matrix = toGlm(source->mTransformation);

    aiVector3D scaling;
    aiVector3D position;
    aiQuaternion rotation;
    source->mTransformation.Decompose(scaling, rotation, position);
    node.translation = {position.x, position.y, position.z};
    node.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};
    node.scale = {scaling.x, scaling.y, scaling.z};
    node.hasMatrix = false;
    mesh.nodes.push_back(node);

    for (unsigned int m = 0; m < source->mNumMeshes; ++m)
    {
        const unsigned int meshIndex = source->mMeshes[m];
        if (meshIndex < mesh.subMeshes.size())
        {
            mesh.subMeshes[meshIndex].nodeIndex = index;
        }
    }

    for (unsigned int i = 0; i < source->mNumChildren; ++i)
    {
        mesh.nodes[index].children.push_back(static_cast<int>(mesh.nodes.size()));
        buildNodes(source->mChildren[i], index, mesh, nodeMap);
    }
}

static int findNode(const Mesh &mesh, const std::string &name)
{
    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) if (mesh.nodes[i].name == name) return static_cast<int>(i);
    return -1;
}

static void buildAnimations(const aiScene *scene, Mesh &mesh)
{
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
    {
        const aiAnimation *source = scene->mAnimations[a];
        Animation animation;
        animation.name = source->mName.length > 0 ? source->mName.C_Str() : "Animation_" + std::to_string(a);
        const double ticksPerSecond = source->mTicksPerSecond > 0.0 ? source->mTicksPerSecond : 25.0;
        animation.duration = static_cast<float>(source->mDuration / ticksPerSecond);

        std::unordered_map<std::string, std::size_t> searchOffset;

        for (unsigned int c = 0; c < source->mNumChannels; ++c)
        {
            const aiNodeAnim *sourceChannel = source->mChannels[c];
            const std::string nodeName = sourceChannel->mNodeName.C_Str();

            int targetNode = -1;
            std::size_t start = searchOffset[nodeName];
            for (std::size_t i = start; i < mesh.nodes.size(); ++i)
            {
                if (mesh.nodes[i].name == nodeName)
                {
                    targetNode = static_cast<int>(i);
                    searchOffset[nodeName] = i + 1;
                    break;
                }
            }
            if (targetNode < 0)
            {
                for (std::size_t i = 0; i < mesh.nodes.size(); ++i)
                {
                    if (mesh.nodes[i].name == nodeName)
                    {
                        targetNode = static_cast<int>(i);
                        break;
                    }
                }
            }
            if (targetNode < 0) continue;

            if (sourceChannel->mNumPositionKeys > 0)
            {
                AnimChannel channel;
                channel.targetNode = targetNode;
                channel.path = AnimTargetPath::Translation;
                for (unsigned int k = 0; k < sourceChannel->mNumPositionKeys; ++k)
                {
                    if (sourceChannel->mPositionKeys[k].mInterpolation == aiAnimInterpolation_Step)
                        channel.interpolation = AnimInterpolation::Step;
                    channel.timestamps.push_back(
                        static_cast<float>(sourceChannel->mPositionKeys[k].mTime / ticksPerSecond));
                    const aiVector3D value = sourceChannel->mPositionKeys[k].mValue;
                    channel.vec3Values.emplace_back(value.x, value.y, value.z);
                }
                animation.channels.push_back(std::move(channel));
            }

            if (sourceChannel->mNumRotationKeys > 0)
            {
                AnimChannel channel;
                channel.targetNode = targetNode;
                channel.path = AnimTargetPath::Rotation;
                for (unsigned int k = 0; k < sourceChannel->mNumRotationKeys; ++k)
                {
                    if (sourceChannel->mRotationKeys[k].mInterpolation == aiAnimInterpolation_Step)
                        channel.interpolation = AnimInterpolation::Step;
                    channel.timestamps.push_back(
                        static_cast<float>(sourceChannel->mRotationKeys[k].mTime / ticksPerSecond));
                    const aiQuaternion value = sourceChannel->mRotationKeys[k].mValue;
                    channel.quatValues.emplace_back(value.w, value.x, value.y, value.z);
                }
                animation.channels.push_back(std::move(channel));
            }

            if (sourceChannel->mNumScalingKeys > 0)
            {
                AnimChannel channel;
                channel.targetNode = targetNode;
                channel.path = AnimTargetPath::Scale;
                for (unsigned int k = 0; k < sourceChannel->mNumScalingKeys; ++k)
                {
                    if (sourceChannel->mScalingKeys[k].mInterpolation == aiAnimInterpolation_Step)
                        channel.interpolation = AnimInterpolation::Step;
                    channel.timestamps.push_back(
                        static_cast<float>(sourceChannel->mScalingKeys[k].mTime / ticksPerSecond));
                    const aiVector3D value = sourceChannel->mScalingKeys[k].mValue;
                    channel.vec3Values.emplace_back(value.x, value.y, value.z);
                }
                animation.channels.push_back(std::move(channel));
            }
        }
        mesh.animations.push_back(std::move(animation));
    }
}

void MeshManager::initFromScene(const aiScene *aiScene, const std::filesystem::path &path, Mesh &mesh,
                                const bool generateMipmap) const
{
    std::unordered_map<const aiNode *, int> nodeMap;
    mesh.subMeshes.resize(aiScene->mNumMeshes);
    buildNodes(aiScene->mRootNode, -1, mesh, nodeMap);
    buildAnimations(aiScene, mesh);

    std::vector meshTransforms(aiScene->mNumMeshes, glm::mat4(1.0f));
    assignNodeTransforms(aiScene->mRootNode, aiMatrix4x4(), meshTransforms);

    for (unsigned int i = 0; i < mesh.subMeshes.size(); i++)
    {
        const aiMesh *aiMesh = aiScene->mMeshes[i];
        Skin *skinPtr = nullptr;
        if (aiMesh->HasBones())
        {
            Skin skin;
            for (unsigned int b = 0; b < aiMesh->mNumBones; ++b)
            {
                const aiBone *bone = aiMesh->mBones[b];
                SkinJoint joint;
                joint.name = bone->mName.C_Str();
                joint.inverseBindMatrix = toGlm(bone->mOffsetMatrix);
                if (bone->mNode && nodeMap.contains(bone->mNode))
                {
                    joint.node = nodeMap[bone->mNode];
                }
                else
                {
                    joint.node = findNode(mesh, joint.name);
                }
                skin.joints.push_back(joint);
            }
            mesh.subMeshes[i].skinIndex = static_cast<int>(mesh.skins.size());
            mesh.skins.push_back(std::move(skin));
            skinPtr = &mesh.skins.back();
        }
        else
        {
            mesh.subMeshes[i].skinIndex = -1;
        }

        initSubMesh(aiScene, aiMesh, path, meshTransforms[i], skinPtr, mesh.subMeshes[i], generateMipmap);
    }
}

void MeshManager::initSubMesh(const aiScene *aiScene, const aiMesh *aiMesh, const std::filesystem::path &path,
                              const glm::mat4 &nodeTransform, const Skin *skin, SubMesh &subMesh,
                              const bool generateMipmap) const
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    subMesh.vertexCount = aiMesh->mNumVertices;
    subMesh.materialId = mMaterialManager.loadMaterial(aiScene, aiScene->mMaterials[aiMesh->mMaterialIndex],
                                                       path, generateMipmap);
    subMesh.nodeTransform = nodeTransform;

    std::vector<Vertex> vertices(aiMesh->mNumVertices);

    for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
    {
        const aiVector3D &pPos = aiMesh->mVertices[i];
        vertices[i].position = glm::vec3(pPos.x, pPos.y, pPos.z);

        const aiVector3D &normal = aiMesh->mNormals != nullptr ? aiMesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
        vertices[i].normal = glm::vec3(normal.x, normal.y, normal.z);

        const aiVector3D &pTexCoord = aiMesh->HasTextureCoords(0) ? aiMesh->mTextureCoords[0][i] : Zero3D;
        vertices[i].texCoords = glm::vec2(pTexCoord.x, pTexCoord.y);
    }

    std::vector<unsigned int> indices;
    indices.reserve(aiMesh->mNumFaces * 3);

    for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
    {
        const aiFace &face = aiMesh->mFaces[i];
        if (face.mNumIndices == 3)
        {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
        else if (face.mNumIndices > 3)
        {
            for (unsigned int j = 1; j + 1 < face.mNumIndices; ++j)
            {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[j]);
                indices.push_back(face.mIndices[j + 1]);
            }
        }
    }

    subMesh.indiceCount = indices.size();

    if (aiMesh->HasBones() && skin != nullptr)
    {
        std::vector<std::array<std::pair<float, unsigned int>, 4>> topK(
            vertices.size(), {{{0.0f, 0}, {0.0f, 0}, {0.0f, 0}, {0.0f, 0}}});

        for (unsigned int b = 0; b < aiMesh->mNumBones; ++b)
        {
            const aiBone *bone = aiMesh->mBones[b];
            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const uint32_t vid = bone->mWeights[w].mVertexId;
                const float weight = bone->mWeights[w].mWeight;
                if (vid >= vertices.size() || weight <= 0.0f) continue;
                auto &slot = topK[vid];
                if (weight > slot.back().first)
                {
                    slot.back() = {weight, b};
                    std::ranges::sort(slot,
                                      [](const auto &left, const auto &right) { return left.first > right.first; });
                }
            }
        }
        for (std::size_t v = 0; v < vertices.size(); ++v)
        {
            float sum = 0.0f;
            for (const auto &weight: topK[v] | std::views::keys) sum += weight;
            if (sum <= 0.0f) { continue; }
            for (std::size_t k = 0; k < 4; ++k)
            {
                vertices[v].weights[k] = topK[v][k].first / sum;
                vertices[v].boneIds[k] = topK[v][k].second;
            }
        }
    }


    CompletableFuture<void>::runAsync([vertices = std::move(vertices), indices = std::move(indices), &subMesh]
    {
        createBuffers(vertices, indices, &subMesh.vao, &subMesh.vbo, &subMesh.ibo);
    }, mEngine.getGlExecutor());
}

void MeshManager::createBuffers(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices,
                                GLuint *vao, GLuint *vbo, GLuint *ibo)
{
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ibo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
    if (!indices.empty())
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
                     GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<GLvoid *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<GLvoid *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<GLvoid *>(offsetof(Vertex, texCoords)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<GLvoid *>(offsetof(Vertex, weights)));
    glEnableVertexAttribArray(3);

    glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(Vertex),
                           reinterpret_cast<GLvoid *>(offsetof(Vertex, boneIds)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) { std::cerr << "OpenGL Error: " << error << std::endl; }
}
