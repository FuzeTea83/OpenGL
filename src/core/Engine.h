#ifndef OPENGL_ENGINE_H
#define OPENGL_ENGINE_H

#include <memory>

#include "Window.h"
#include "core/input/Input.h"
#include "core/renderer/GuiRenderer.h"
#include "core/renderer/SceneRenderer.h"
#include "ecs/core/ECS.h"
#include "resource/material/MaterialManager.h"
#include "resource/mesh/MeshManager.h"
#include "resource/scene/SceneManager.h"
#include "resource/shader/ShaderManager.h"
#include "resource/texture/TextureManager.h"

class Engine
{
public:
    ~Engine();

    void start();

    void stop();

    Window &getWindow() const { return *mWindow; }

    Input &getInput() const { return *mInput; }

    TextureManager &getTextureManager() const { return *mTextureManager; }

    MaterialManager &getMaterialManager() const { return *mMaterialManager; }

    MeshManager &getMeshManager() const { return *mMeshManager; }

    ShaderManager &getShaderManager() const { return *mShaderManager; }

    ECS &getECS() const { return *mECS; }

    SceneRenderer &getSceneRenderer() const { return *mSceneRenderer; }

    GuiRenderer &getGuiRenderer() const { return *mGuiRenderer; }

    SceneManager &getSceneLoader() const { return *mSceneLoader; }

    bool isRunning() const { return !mWindow->shouldClose() && mRunning; }

    Executor &getGlExecutor() const { return *mGlExecutor; }

    Executor &getEngineExecutor() const { return *mEngineExecutor; }


    template<typename T>
    T runOnGlThread(const std::function<T()>& func) const
    {
        if (glfwGetCurrentContext() != nullptr)
        {
            return func();
        }
        return CompletableFuture<T>::runAsync([func]
        {
            return func();
        }, *mGlExecutor).get();
    }

private:
    void run();

    void update(float deltaTime) const;

    void render(float alpha) const;

    std::unique_ptr<Executor> mEngineExecutor;
    std::unique_ptr<Executor> mGlExecutor;
    std::unique_ptr<Window> mWindow;
    std::unique_ptr<Input> mInput;
    std::unique_ptr<TextureManager> mTextureManager;
    std::unique_ptr<MaterialManager> mMaterialManager;
    std::unique_ptr<MeshManager> mMeshManager;
    std::unique_ptr<ShaderManager> mShaderManager;
    std::unique_ptr<SceneManager> mSceneLoader;
    std::unique_ptr<ECS> mECS;
    std::unique_ptr<SceneRenderer> mSceneRenderer;
    std::unique_ptr<GuiRenderer> mGuiRenderer;
    bool mRunning{false};
};
#endif //OPENGL_ENGINE_H
