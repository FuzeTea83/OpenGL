#ifndef OPENGL_SCENE_RENDERER_H
#define OPENGL_SCENE_RENDERER_H

#include <algorithm>
#include <array>
#include <vector>

#include "core/Window.h"
#include "ecs/component/TransformComponent.h"
#include "glad/gl.h"
#include "resource/material/Material.h"
#include "resource/shader/FrameBuffer.h"
#include "resource/shader/Shader.h"

#define MAX_LIGHTS 128
#define MAX_BONES 512

struct alignas(16) MaterialUniforms
{
    glm::vec4 ambient{0.0f};    // xyz = Ambient Color, w = Is PBR Material
    glm::vec4 diffuse{0.0f};    // xyz = Diffuse/Base Color, w = Has Diffuse Map
    glm::vec4 specular{0.0f};   // xyz = Specular Color, w = Has Specular Map
    glm::vec4 emissive{0.0f};   // xyz = Emissive Color, w = Intensity
    glm::vec4 pbrFactors{0.0f}; // x = Roughness, y = Metallic, z = AO, w = Shininess
    glm::vec4 pbrMaps{0.0f};    // x = hasNormalMap, y = hasMetallicRoughnessMap, z = hasEmissiveMap, w = hasAoMap

    MaterialUniforms() = default;

    explicit MaterialUniforms(const Material &material, glm::vec3 emissiveColor = glm::vec3(0.0f),
                              float emissiveIntensity = 0.0f);
};

struct alignas(16) LightUniforms
{
    glm::vec4 position{0};    // xyz = Position, w = Light Type (0 = directional, 1 = point, 2 = spot, 3 = neon)
    glm::vec4 color{1};       // xyz = Color, w = Specular Factor (Phong Legacy)
    glm::vec4 direction{0};   // xyz = Direction, w = Ambient Factor (Phong legacy)
    glm::vec4 attenuation{1}; // x = Constant, y = Linear, z = Quadratic, w = Diffuse Factor (Phong legacy)
    glm::vec4 spotParams{0};  // x = Cos Inner, y = Cos Outer, z = Size, w = Intensity (PBR)
};


struct DrawCommand
{
    ShaderId shaderId;
    GLuint vaoId;
    uint32_t indexCount{0};
    uint32_t vertexCount{0};

    bool outline{false};

    glm::mat4 model;

    Material material;
    const std::vector<glm::mat4> *boneMatrices = nullptr;
    glm::vec3 emissiveColor{0.0f};
    float emissiveIntensity{0.0f};
    bool emissivePartsOnly{true};
    bool emissiveOnly{false};

    uint64_t sort_key() const
    {
        return (static_cast<uint64_t>(shaderId) << 32) |
               static_cast<uint64_t>(vaoId & 0xFFFFFFFF);
    }
};

struct ShaderConfig
{
    struct MainShaderConfig
    {
        glm::vec3 globalAmbient{0.0f};
    } mainShader;

    struct SkyboxShaderConfig
    {
        glm::vec3 skyColor{1.0f};
        float skyIntensity{1.0f};
        glm::vec3 skyRotation{0.0f, 0.0f, 0.0f};
    } skyboxShader;

    struct BrightShaderConfig
    {
        float threshold{5.0f};
        float knee{2.0f};
    } brightShader;

    struct BlurShaderConfig
    {
        float initialStrength{0.4f};
        float strengthMultiplier{1.1f};
        int passes{4};
    } blurShader;

    struct CompositeShaderConfig
    {
        float exposure{1.0f};
    } compositeShader;

    struct OutlineShaderConfig
    {
        glm::vec4 color{1.0f, 0.8f, 0.0f, 1.0f};
        float width{3.0f};
    } outlineShader;

    Codec codec = Codec()
                  .addColor3("Global Ambient", &mainShader.globalAmbient)
                  .addColor3("Sky Color", &skyboxShader.skyColor)
                  .addFloat("Sky Intensity", &skyboxShader.skyIntensity)
                  .addVec3("Sky Rotation", &skyboxShader.skyRotation)
                  .addFloat("Bright Threshold", &brightShader.threshold)
                  .addFloat("Bright Knee", &brightShader.knee)
                  .addFloat("Blur Initial Strength", &blurShader.initialStrength)
                  .addFloat("Blur Strength Multiplier", &blurShader.strengthMultiplier)
                  .addInt("Blur Passes", &blurShader.passes)
                  .addFloat("Composite Exposure", &compositeShader.exposure)
                  .addColor4("Outline Color", &outlineShader.color)
                  .addFloat("Outline Width", &outlineShader.width);
};

enum class ShaderType
{
    Main,
    Skybox,
    Bright,
    Blur,
    OutlineMask,
    White,
    Composite,
    COUNT
};

class SceneRenderer
{
public:
    explicit SceneRenderer(Engine &engine);

    ~SceneRenderer();

    static void clear();

    void render();

    ShaderId getShaderId(ShaderType type) const { return mShaderIds[static_cast<std::size_t>(type)]; }

    void setSkyboxTexture(TextureId id);

    void setGlobalUniforms(const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &viewTranslated,
                           const glm::vec3 &cameraPosition);

    void setLightUniforms(const std::array<LightUniforms, MAX_LIGHTS> &lights, int lightCount) const;

    void addDrawCommand(const DrawCommand &drawCommand) { mDrawCommands.push_back(drawCommand); }

    ShaderConfig &getShaderConfig() { return mShaderConfig; }

private:
    enum class Bindings { Light = 0, Material = 1, Bone = 2 };

    enum class Fbo { Hdr, Bloom0, Bloom1, OutlineMask, COUNT };

    void drawFullScreenQuad() const;

    void initQuad();

    void executeDrawCommands();

    void renderSkybox() const;

    const Shader &getShader(ShaderType type) const;

    static void createUbo(const Shader &shader, GLuint *ubo, GLsizeiptr size, const std::string &name,
                          Bindings bindings);

    void createSkyboxBuffers();

    void executeOutlineMaskPass() const;

    void applyPostProcess() const;

    const FrameBuffer &getFbo(const Fbo type) const { return mFbo[static_cast<std::size_t>(type)]; }

    Engine &mEngine;
    GLuint mMaterialUbo{0};
    GLuint mLightUbo{0};
    GLuint mBoneUbo{0};
    std::vector<DrawCommand> mDrawCommands;

    struct GlobalUniforms
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 viewTranslated;
        glm::vec3 cameraPosition;
        bool isCameraPositionSet{false};
    } mGlobalUniforms;

    ShaderId mShaderIds[static_cast<std::size_t>(ShaderType::COUNT)] = {};
    FrameBuffer mFbo[static_cast<std::size_t>(Fbo::COUNT)] = {};
    ShaderConfig mShaderConfig;

    GLuint mQuadVAO = 0;
    GLuint mQuadVBO = 0;

    struct Skybox
    {
        GLuint vbo = 0;
        GLuint vao = 0;
        GLuint ibo = 0;
        GLuint texture = 0;
        bool hasSkybox = false;
    } mSkybox;
};


#endif //OPENGL_SCENE_RENDERER_H
