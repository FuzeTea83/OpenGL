#include "SceneRenderer.h"

#include <cstring>
#include <iostream>

#include "core/Engine.h"
#include "core/Window.h"
#include "utils/OpenGLUtils.h"

#define SHADER_MAIN_VERT "assets/shaders/main.vert"
#define SHADER_MAIN_FRAG "assets/shaders/main.frag"
#define SHADER_WHITE_FRAG "assets/shaders/white.frag"
#define SHADER_SKYBOX_VERT "assets/shaders/skybox.vert"
#define SHADER_SKYBOX_FRAG "assets/shaders/skybox.frag"
#define SHADER_POSTPROCESS_QUAD_VERT "assets/shaders/postprocess/quad.vert"
#define SHADER_POSTPROCESS_BLUR_FRAG "assets/shaders/postprocess/blur.frag"
#define SHADER_POSTPROCESS_BRIGHT_FRAG "assets/shaders/postprocess/bright.frag"
#define SHADER_POSTPROCESS_OUTLINE_MASK_FRAG "assets/shaders/postprocess/outline_mask.frag"
#define SHADER_POSTPROCESS_COMPOSITE_FRAG "assets/shaders/postprocess/composite.frag"

#define DEBUG_RENDER

static constexpr glm::vec3 skyboxVertices[] = {
    // Front Face (Z+1)
    glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),

    // Back Face (Z-1)
    glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),

    // Top Face (Y+1)
    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, 1.0f),

    // Down Face (Y-1)
    glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f),

    // Right Face (X+1)
    glm::vec3(1.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f),
    glm::vec3(1.0f, -1.0f, -1.0f),
    glm::vec3(1.0f, 1.0f, -1.0f),

    // Left Face (X-1)
    glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f),
    glm::vec3(-1.0f, -1.0f, 1.0f),
};

static constexpr unsigned int skyboxIndices[] = {
    // Front Face
    0, 1, 2, 2, 3, 0,
    // Back Face
    4, 5, 6, 6, 7, 4,
    // Up Face
    8, 9, 10, 10, 11, 8,
    // Down Face
    12, 13, 14, 14, 15, 12,
    // Left Face
    16, 17, 18, 18, 19, 16,
    // Right Face
    20, 21, 22, 22, 23, 20
};

MaterialUniforms::MaterialUniforms(const Material &material, const glm::vec3 emissiveColor,
                                   const float emissiveIntensity)
{
    ambient = glm::vec4(material.ambientColor, material.isPBR ? 1.0f : 0.0f);
    diffuse = glm::vec4(material.diffuseColor, material.diffuseMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f);
    specular = glm::vec4(material.specularColor, material.specularMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f);
    emissive = glm::vec4(material.emissiveColor + emissiveColor, material.intensity + emissiveIntensity);
    pbrFactors = glm::vec4(material.roughness, material.metallic, material.ao, material.shininess);
    pbrMaps = glm::vec4(
        material.normalMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f,
        material.metallicRoughnessMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f,
        material.emissiveMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f,
        material.aoMap != INVALID_TEXTURE_ID ? 1.0f : 0.0f
    );
}

SceneRenderer::SceneRenderer(Engine &engine) : mEngine(engine)
{
    assertGlThread();
    Window &window = engine.getWindow();
    ShaderManager &shaderManager = engine.getShaderManager();

    window.addFramebufferSizeCallback([this](const int width, const int height)
    {
        mEngine.runOnGlThread<void>([this, width, height]
        {
            for (std::size_t i = 0; i < static_cast<std::size_t>(Fbo::COUNT); ++i)
            {
                mFbo[i].resize(width, height);
            }
        });
    });

    mFbo[static_cast<std::size_t>(Fbo::Hdr)] = FrameBuffer(window.getWidth(), window.getHeight(), true, true);
    mFbo[static_cast<std::size_t>(Fbo::Bloom0)] = FrameBuffer(window.getWidth(), window.getHeight(), true, true);
    mFbo[static_cast<std::size_t>(Fbo::Bloom1)] = FrameBuffer(window.getWidth(), window.getHeight(), true, true);
    mFbo[static_cast<std::size_t>(Fbo::OutlineMask)] = FrameBuffer(window.getWidth(), window.getHeight(), false, true);

    mShaderIds[static_cast<std::size_t>(ShaderType::Main)] = shaderManager.loadShader(
        SHADER_MAIN_VERT, SHADER_MAIN_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::Skybox)] = shaderManager.loadShader(
        SHADER_SKYBOX_VERT, SHADER_SKYBOX_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::Bright)] = shaderManager.loadShader(
        SHADER_POSTPROCESS_QUAD_VERT, SHADER_POSTPROCESS_BRIGHT_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::Blur)] = shaderManager.loadShader(
        SHADER_POSTPROCESS_QUAD_VERT, SHADER_POSTPROCESS_BLUR_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::OutlineMask)] = shaderManager.loadShader(
        SHADER_POSTPROCESS_QUAD_VERT, SHADER_POSTPROCESS_OUTLINE_MASK_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::White)] = shaderManager.loadShader(
        SHADER_MAIN_VERT, SHADER_WHITE_FRAG);
    mShaderIds[static_cast<std::size_t>(ShaderType::Composite)] = shaderManager.loadShader(
        SHADER_POSTPROCESS_QUAD_VERT, SHADER_POSTPROCESS_COMPOSITE_FRAG);


    const Shader &mainShader = getShader(ShaderType::Main);

    initQuad();

    createUbo(mainShader, &mLightUbo, sizeof(LightUniforms) * MAX_LIGHTS + sizeof(int), "LightBlock", Bindings::Light);
    createUbo(mainShader, &mMaterialUbo, sizeof(MaterialUniforms), "MaterialBlock", Bindings::Material);
    createUbo(mainShader, &mBoneUbo, sizeof(glm::mat4) * MAX_BONES, "BoneBlock", Bindings::Bone);
    createSkyboxBuffers();
}

SceneRenderer::~SceneRenderer()
{
    if (mLightUbo != 0) { glDeleteBuffers(1, &mLightUbo); }
    if (mMaterialUbo != 0) { glDeleteBuffers(1, &mMaterialUbo); }
    if (mBoneUbo != 0) { glDeleteBuffers(1, &mBoneUbo); }
    if (mSkybox.vbo != 0) { glDeleteBuffers(1, &mSkybox.vbo); }
    if (mSkybox.vao != 0) { glDeleteVertexArrays(1, &mSkybox.vao); }
    if (mSkybox.ibo != 0) { glDeleteBuffers(1, &mSkybox.ibo); }
    if (mQuadVAO != 0) { glDeleteVertexArrays(1, &mQuadVAO); }
    if (mQuadVBO != 0) { glDeleteBuffers(1, &mQuadVBO); }
}

void SceneRenderer::clear()
{
    assertGlThread();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SceneRenderer::render()
{
    assertGlThread();
    if (mGlobalUniforms.isCameraPositionSet) { getFbo(Fbo::Hdr).bind(); }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!mGlobalUniforms.isCameraPositionSet) { return; }
    renderSkybox();
    executeDrawCommands();
    executeOutlineMaskPass();
    FrameBuffer::unbind();
    applyPostProcess();
    mDrawCommands.clear();
    mGlobalUniforms.isCameraPositionSet = false;
}

void SceneRenderer::setSkyboxTexture(const TextureId id)
{
    if (id == INVALID_TEXTURE_ID)
    {
        mSkybox.hasSkybox = false;
        mSkybox.texture = 0;
        return;
    }
    mSkybox.hasSkybox = true;
    mSkybox.texture = mEngine.getTextureManager().getTexture(id).glId;
}

void SceneRenderer::setGlobalUniforms(const glm::mat4 &projection, const glm::mat4 &view, const glm::mat4 &viewTranslated,
                                 const glm::vec3 &cameraPosition)
{
    mEngine.runOnGlThread<void>([this, projection, view, viewTranslated, cameraPosition]
    {
        mGlobalUniforms.projection = projection;
        mGlobalUniforms.view = view;
        mGlobalUniforms.viewTranslated = viewTranslated;
        mGlobalUniforms.cameraPosition = cameraPosition;
        mGlobalUniforms.isCameraPositionSet = true;
    });
}

void SceneRenderer::setLightUniforms(const std::array<LightUniforms, 128> &lights, const int lightCount) const
{
    assertGlThread();
    glBindBuffer(GL_UNIFORM_BUFFER, mLightUbo);
    const int count = std::clamp(lightCount, 0, MAX_LIGHTS);
    if (count > 0) { glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUniforms) * count, lights.data()); }
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(LightUniforms) * MAX_LIGHTS, sizeof(int), &count);
    glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(Bindings::Light), mLightUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SceneRenderer::drawFullScreenQuad() const
{
    assertGlThread();
    glBindVertexArray(mQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void SceneRenderer::initQuad()
{
    assertGlThread();
    glGenVertexArrays(1, &mQuadVAO);
    glGenBuffers(1, &mQuadVBO);

    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);

    constexpr float vertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void *>(nullptr));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<GLvoid *>(2 * sizeof(float)));

    glBindVertexArray(0);
}

void SceneRenderer::executeDrawCommands()
{
    assertGlThread();
    std::ranges::sort(mDrawCommands, [](const DrawCommand &a, const DrawCommand &b)
    {
        return a.sort_key() < b.sort_key();
    });

    std::ranges::sort(mDrawCommands, [](const DrawCommand &a, const DrawCommand &b)
    {
        return a.material.transparent < b.material.transparent;
    });

    const ShaderManager &shaderManager = mEngine.getShaderManager();
    TextureManager &textureManager = mEngine.getTextureManager();
    ShaderId lastShader{INVALID_SHADER_ID};
    GLuint lastVao = 0;
    const std::vector<glm::mat4> *lastBoneMatrices = nullptr;

    MaterialUniforms lastMaterial{};

    for (const auto &cmd: mDrawCommands)
    {
        if (cmd.shaderId != lastShader)
        {
            const Shader &shader = shaderManager.getShader(cmd.shaderId);

            shader.use();

            shader.setUniformMat4("projection", mGlobalUniforms.projection);
            shader.setUniformMat4("view", mGlobalUniforms.viewTranslated);
            shader.setUniform("viewPos", mGlobalUniforms.cameraPosition);
            shader.setUniform("globalAmbient", mShaderConfig.mainShader.globalAmbient);

            shader.setUniformSampler2D("diffuseMap", 0);
            shader.setUniformSampler2D("specularMap", 1);
            shader.setUniformSampler2D("normalMap", 2);
            shader.setUniformSampler2D("metallicRoughnessMap", 3);
            shader.setUniformSampler2D("emissiveMap", 4);
            shader.setUniformSampler2D("aoMap", 5);

            lastShader = cmd.shaderId;
            lastVao = 0;
            lastBoneMatrices = nullptr;
        }

        const Shader &shader = shaderManager.getShader(cmd.shaderId);
        shader.setUniformMat4("model", cmd.model);

        if (cmd.boneMatrices && !cmd.boneMatrices->empty())
        {
            shader.setUniform("hasSkinning", 1);
            if (cmd.boneMatrices != lastBoneMatrices)
            {
                glBindBuffer(GL_UNIFORM_BUFFER, mBoneUbo);
                const std::size_t count = std::min(cmd.boneMatrices->size(), static_cast<std::size_t>(MAX_BONES));
                glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(glm::mat4), cmd.boneMatrices->data());
                glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(Bindings::Bone), mBoneUbo);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
                lastBoneMatrices = cmd.boneMatrices;
            }
        }
        else
        {
            shader.setUniform("hasSkinning", 0);
            lastBoneMatrices = nullptr;
        }

        if (cmd.vaoId != lastVao)
        {
            glBindVertexArray(cmd.vaoId);
            lastVao = cmd.vaoId;
        }

        MaterialUniforms material(cmd.material, cmd.emissivePartsOnly && glm::length(cmd.material.emissiveColor) < 0.01f ? glm::vec3(0.0f) : cmd.emissiveColor , cmd.emissiveIntensity);
        shader.setUniform("emissiveOnly", cmd.emissiveOnly);
        shader.setUniform("alphaMasked", cmd.material.alphaMasked);
        shader.setUniform("alphaCutoff", cmd.material.alphaCutoff);

        if (std::memcmp(&material, &lastMaterial, sizeof(MaterialUniforms)) != 0)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, mMaterialUbo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(MaterialUniforms), &material);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            lastMaterial = material;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmd.material.diffuseMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.diffuseMap).glId
                                         : 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cmd.material.specularMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.specularMap).glId
                                         : 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, cmd.material.normalMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.normalMap).glId
                                         : 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, cmd.material.metallicRoughnessMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.metallicRoughnessMap).glId
                                         : 0);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, cmd.material.emissiveMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.emissiveMap).glId
                                         : 0);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, cmd.material.aoMap != INVALID_TEXTURE_ID
                                         ? textureManager.getTexture(cmd.material.aoMap).glId
                                         : 0);

        if (cmd.material.transparent)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        if (cmd.indexCount > 0)
        {
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cmd.indexCount), GL_UNSIGNED_INT, nullptr);
        }
        else if (cmd.vertexCount > 0) { glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(cmd.vertexCount)); }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}


void SceneRenderer::renderSkybox() const
{
    assertGlThread();
    if (!mSkybox.hasSkybox || mSkybox.texture == 0) { return; }
    const Shader &shader = getShader(ShaderType::Skybox);

    shader.use();

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    shader.setUniformMat4("view", mGlobalUniforms.view);
    shader.setUniformMat4("projection", mGlobalUniforms.projection);
    shader.setUniformSampler2D("skybox", 0);
    shader.setUniform("skyColor", mShaderConfig.skyboxShader.skyColor);
    shader.setUniform("skyIntensity", mShaderConfig.skyboxShader.skyIntensity);

    auto skyRotation = glm::mat4(1.0f);
    skyRotation = glm::rotate(skyRotation, glm::radians(mShaderConfig.skyboxShader.skyRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    skyRotation = glm::rotate(skyRotation, glm::radians(mShaderConfig.skyboxShader.skyRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    skyRotation = glm::rotate(skyRotation, glm::radians(mShaderConfig.skyboxShader.skyRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    shader.setUniformMat3("skyRotation", glm::mat3(skyRotation));


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mSkybox.texture);

    glBindVertexArray(mSkybox.vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

const Shader &SceneRenderer::getShader(ShaderType type) const
{
    return mEngine.getShaderManager().getShader(mShaderIds[static_cast<std::size_t>(type)]);
}   

void SceneRenderer::createUbo(const Shader &shader, GLuint *ubo, const GLsizeiptr size, const std::string &name,
                         Bindings bindings)
{
    assertGlThread();
    glGenBuffers(1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(bindings), *ubo);

    const GLuint blockIndex = glGetUniformBlockIndex(shader.programId, name.c_str());
    if (blockIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(shader.programId, blockIndex, static_cast<GLuint>(bindings));
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SceneRenderer::createSkyboxBuffers()
{
    assertGlThread();
    glGenVertexArrays(1, &mSkybox.vao);
    glGenBuffers(1, &mSkybox.vbo);
    glGenBuffers(1, &mSkybox.ibo);

    glBindVertexArray(mSkybox.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mSkybox.vbo);
    glBufferData(GL_ARRAY_BUFFER, std::size(skyboxVertices) * sizeof(glm::vec3), skyboxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mSkybox.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, std::size(skyboxIndices) * sizeof(unsigned int), skyboxIndices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<GLvoid *>(nullptr));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void SceneRenderer::executeOutlineMaskPass() const
{
    assertGlThread();
    auto &maskFbo = getFbo(Fbo::OutlineMask);
    maskFbo.bind();
    glViewport(0, 0, maskFbo.getWidth(), maskFbo.getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    const Shader &maskShader = getShader(ShaderType::White);
    maskShader.use();
    maskShader.setUniformMat4("projection", mGlobalUniforms.projection);
    maskShader.setUniformMat4("view", mGlobalUniforms.viewTranslated);

    GLuint lastVao = 0;
    const std::vector<glm::mat4> *lastBoneMatrices = nullptr;

    for (const auto &cmd: mDrawCommands)
    {
        if (!cmd.outline) continue;

        maskShader.setUniformMat4("model", cmd.model);

        if (cmd.boneMatrices && !cmd.boneMatrices->empty())
        {
            maskShader.setUniform("hasSkinning", 1);
            if (cmd.boneMatrices != lastBoneMatrices)
            {
                glBindBuffer(GL_UNIFORM_BUFFER, mBoneUbo);
                const std::size_t count = std::min(cmd.boneMatrices->size(),
                                              static_cast<std::size_t>(MAX_BONES));
                glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(glm::mat4),
                                cmd.boneMatrices->data());
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
                lastBoneMatrices = cmd.boneMatrices;
            }
        }
        else
        {
            maskShader.setUniform("hasSkinning", 0);
            lastBoneMatrices = nullptr;
        }

        if (cmd.vaoId != lastVao)
        {
            glBindVertexArray(cmd.vaoId);
            lastVao = cmd.vaoId;
        }

        if (cmd.indexCount > 0)
        {
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cmd.indexCount),
                           GL_UNSIGNED_INT, nullptr);
        }
        else if (cmd.vertexCount > 0)
        {

            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(cmd.vertexCount));
        }

    }

    maskFbo.unbind();
}


void SceneRenderer::applyPostProcess() const
{
    assertGlThread();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    getFbo(Fbo::Bloom0).bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const Shader &brightShader = getShader(ShaderType::Bright);
    brightShader.use();
    brightShader.setUniform("threshold", mShaderConfig.brightShader.threshold);
    brightShader.setUniform("knee", mShaderConfig.brightShader.knee);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getFbo(Fbo::Hdr).getTexture());
    brightShader.setUniform("scene", 0);
    drawFullScreenQuad();
    getFbo(Fbo::Bloom0).unbind();

    const Shader &blurShader = getShader(ShaderType::Blur);
    blurShader.use();
    bool horizontal = true, first_iteration = true;
    float blurStrength = mShaderConfig.blurShader.initialStrength;
    for (int i = 0; i < mShaderConfig.blurShader.passes; ++i)
    {
        getFbo(horizontal ? Fbo::Bloom1 : Fbo::Bloom0).bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::vec2 direction = horizontal
                                  ? glm::vec2(blurStrength / static_cast<float>(getFbo(Fbo::Bloom1).getWidth()),
                                              0.0f)
                                  : glm::vec2(
                                      0.0f, blurStrength / static_cast<float>(getFbo(Fbo::Bloom0).getHeight()));
        blurShader.setUniform("direction", direction);

        blurStrength *= mShaderConfig.blurShader.strengthMultiplier;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration
                                         ? getFbo(Fbo::Bloom0).getTexture()
                                         : getFbo(!horizontal ? Fbo::Bloom1 : Fbo::Bloom0).getTexture());
        blurShader.setUniform("image", 0);
        drawFullScreenQuad();

        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }
    getFbo(Fbo::Bloom0).unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, getFbo(Fbo::Hdr).getWidth(), getFbo(Fbo::Hdr).getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const Shader &compositeShader = getShader(ShaderType::Composite);
    compositeShader.use();
    compositeShader.setUniform("exposure", mShaderConfig.compositeShader.exposure);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getFbo(Fbo::Hdr).getTexture());
    compositeShader.setUniform("scene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, getFbo(Fbo::Bloom0).getTexture());
    compositeShader.setUniform("bloom", 1);

    drawFullScreenQuad();

    const Shader &outlineShader = getShader(ShaderType::OutlineMask);
    outlineShader.use();
    outlineShader.setUniformSampler2D("maskTexture", 0);
    outlineShader.setUniform("pixelSize", glm::vec2(
                                 1.0f / static_cast<float>(getFbo(Fbo::OutlineMask).getWidth()),
                                 1.0f / static_cast<float>(getFbo(Fbo::OutlineMask).getHeight())));
    outlineShader.setUniform("outlineColor", mShaderConfig.outlineShader.color);
    outlineShader.setUniform("outlineWidth", mShaderConfig.outlineShader.width);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getFbo(Fbo::OutlineMask).getTexture());
    drawFullScreenQuad();
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}
