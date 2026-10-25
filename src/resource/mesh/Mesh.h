#ifndef OPENGL_MESH_H
#define OPENGL_MESH_H

#include <array>
#include <string>
#include <vector>
#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include "resource/material/Material.h"

using MeshId = unsigned int;
constexpr MeshId INVALID_MESH_ID = 0;

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoords{};
    std::array<unsigned int, 4> boneIds{{0, 0, 0, 0}};
    std::array<float, 4> weights{{0.f, 0.f, 0.f, 0.f}};
};

struct SubMesh
{
    GLuint vao{0}, vbo{0}, ibo{0};
    std::uint32_t indiceCount{0}, vertexCount{0};
    MaterialId materialId{0};
    glm::mat4 nodeTransform{1.0f};
    int skinIndex{-1};
    int nodeIndex{-1};
};

struct Node
{
    std::string name;
    int parent{-1};
    std::vector<int> children;
    glm::mat4 matrix{1.0f};
    glm::vec3 translation{0.0f};
    glm::quat rotation{1, 0, 0, 0};
    glm::vec3 scale{1.0f};
    bool hasMatrix{false};
};

struct SkinJoint
{
    std::string name;
    int node{-1};
    glm::mat4 inverseBindMatrix{1.0f};
};

struct Skin
{
    std::vector<SkinJoint> joints;
};

enum class AnimTargetPath { Translation, Rotation, Scale };

enum class AnimInterpolation { Linear, Step };

struct AnimChannel
{
    int targetNode{-1};
    AnimTargetPath path{AnimTargetPath::Translation};
    AnimInterpolation interpolation{AnimInterpolation::Linear};
    std::vector<float> timestamps;
    std::vector<glm::vec3> vec3Values;
    std::vector<glm::quat> quatValues;
};

struct Animation
{
    std::string name;
    float duration{0};
    std::vector<AnimChannel> channels;
};

struct Mesh
{
    std::vector<SubMesh> subMeshes;
    std::vector<Node> nodes;
    std::vector<Skin> skins;
    std::vector<Animation> animations;
};

struct MeshDefinition
{
    std::string name;
    std::filesystem::path path;
    MeshId id;
    bool generateMipmap;
};

#endif //OPENGL_MESH_H
