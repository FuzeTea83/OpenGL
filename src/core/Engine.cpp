#include "Engine.h"

#include <iostream>
#include <memory>
#include <GLFW/glfw3.h>

#include "ecs/component/ModelAnimator.h"
#include "ecs/system/render/RenderLightSystem.h"
#include "ecs/system/render/RenderMainCameraSystem.h"
#include "ecs/system/render/RenderModelSystem.h"
#include "ecs/system/update/UpdateModelAnimationSystem.h"
#include "ecs/system/update/UpdateCameraInputSystem.h"
#include "ecs/system/update/UpdateAnimatorSystem.h"
#include "ecs/system/update/UpdatePositionSystem.h"
#include "gui/window/AnimatorWindow.h"
#include "gui/window/CameraWindow.h"
#include "gui/window/FileExplorerWindow.h"
#include "gui/window/LoadingWindow.h"
#include "gui/window/ModelImporterWindow.h"
#include "gui/window/PropertyWindow.h"
#include "gui/window/SceneEditorWindow.h"
#include "gui/window/ShaderConfigWindow.h"
#include "gui/window/SkyboxImporter.h"

#define GL_EXECUTOR_THREADS 1
#define ENGINE_EXECUTOR_THREADS 4

Engine::~Engine()
{
    if (mGlExecutor != nullptr)
    {
        runOnGlThread<void>([this]
        {
            mWindow.reset();
            mShaderManager.reset();
            mGuiRenderer.reset();
            mSceneRenderer.reset();
            glfwMakeContextCurrent(nullptr);
        });
        mGlExecutor->shutdown();
        mGlExecutor.reset();
    }
    if (mEngineExecutor != nullptr)
    {
        mInput.reset();
        mECS.reset();
        mMaterialManager.reset();
        mMeshManager.reset();
        mTextureManager.reset();
        mSceneLoader.reset();
        mEngineExecutor->shutdown();
        mEngineExecutor.reset();
    }
}

void Engine::start()
{
    std::cout << "Starting engine..." << std::endl;

    std::cout << "Initializing Executors..." << std::endl;
    mEngineExecutor = std::make_unique<Executor>(ENGINE_EXECUTOR_THREADS);
    mGlExecutor = std::make_unique<Executor>(GL_EXECUTOR_THREADS);

    std::cout << "Initializing window..." << std::endl;
    mWindow = std::make_unique<Window>(*this);

    std::cout << "Initializing input..." << std::endl;
    mInput = std::make_unique<Input>(*this, *mWindow);

    std::cout << "Initializing resource managers..." << std::endl;
    mTextureManager = std::make_unique<TextureManager>(*this);
    mMaterialManager = std::make_unique<MaterialManager>(*mTextureManager);
    mMeshManager = std::make_unique<MeshManager>(*this, *mMaterialManager);
    mShaderManager = std::make_unique<ShaderManager>();

    std::cout << "Initializing scene loader..." << std::endl;
    mSceneLoader = std::make_unique<SceneManager>(*this);

    std::cout << "Initializing ECS..." << std::endl;
    mECS = std::make_unique<ECS>(*this);

    mECS->createSystem<UpdateCameraInputSystem>(0);
    mECS->createSystem<UpdateAnimatorSystem>(1);
    mECS->createSystem<UpdateModelAnimationSystem>(2);
    mECS->createSystem<UpdatePositionSystem>(3);
    mECS->createSystem<RenderMainCameraSystem>(0);
    mECS->createSystem<RenderLightSystem>(1);
    mECS->createSystem<RenderModelSystem>(2);

    mECS->registerComponent<ModelAnimator>("ModelAnimator");
    mECS->registerComponent<CameraComponent>("Camera");
    mECS->registerComponent<MainCameraTag>("MainCameraTag");
    mECS->registerComponent<LightComponent>("Light");
    mECS->registerComponent<ModelComponent>("Model");
    mECS->registerComponent<TransformComponent>("Transform");
    mECS->registerComponent<Animator>("Animator");

    CompletableFuture<void>::runAsync(
        [this]
        {
            std::cout << "Initializing scene renderer..." << std::endl;
            mSceneRenderer = std::make_unique<SceneRenderer>(*this);

            std::cout << "Initializing gui renderer..." << std::endl;
            mGuiRenderer = std::make_unique<GuiRenderer>(*this);

            mGuiRenderer->createMenu("File", 0);
            mGuiRenderer->createMenu("Assets", 2);
            mGuiRenderer->registerWindow<ShaderConfigWindow>();
            mGuiRenderer->registerWindow<CameraWindow>();
            mGuiRenderer->registerWindow<SceneEditorWindow>();
            mGuiRenderer->registerWindow<PropertyWindow>();
            mGuiRenderer->registerWindow<AnimatorWindow>();
            mGuiRenderer->registerWindow<FileExplorerWindow>(false);
            mGuiRenderer->registerWindow<ModelImporterWindow>(false);
            mGuiRenderer->registerWindow<SkyboxImporter>(false);
            mGuiRenderer->registerWindow<LoadingWindow>(false);
        }, *mGlExecutor).get();

    std::cout << "Engine started!" << std::endl;
    CompletableFuture<void>::runAsync([this] { run(); }, *mEngineExecutor).get();
}

void Engine::stop()
{
    mRunning = false;
    std::cout << "Stopping engine..." << std::endl;
}

void Engine::run()
{
    mRunning = true;

    constexpr double timeStep = 1.0 / 60.0;
    double accumulator = 0.0;
    double lastTime = glfwGetTime();
    double lastFpsTime = lastTime;
    int frameCount = 0;

    while (isRunning())
    {
        const double currentTime = glfwGetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;

        if (frameTime > 0.25) frameTime = 0.25;
        accumulator += frameTime;

        frameCount++;
        if (currentTime - lastFpsTime >= 1.0)
        {
            const double fps = static_cast<double>(frameCount) / (currentTime - lastFpsTime);
            const double ms = 1000.0 / fps;
            std::cout << "FPS: " << frameCount << " (" << ms << " ms)" << std::endl;
            frameCount = 0;
            lastFpsTime = currentTime;
        }

        mInput->updateKeyboard();

        runOnGlThread<void>([] { glfwPollEvents(); });

        if (mInput->isPressedThisFrame(Input::Action::ToggleMouseLock)) { mInput->toggleMouseLock(); }
        if (mInput->isPressedThisFrame(Input::Action::ToggleWireFrame)) { mWindow->toggleWireframe(); }
        if (mInput->isPressedThisFrame(Input::Action::ToggleEditor))
        {
            mGuiRenderer->toggleEditor();
            const bool editorVisible = mGuiRenderer->isEditorVisible();
            mInput->lockMouse(!editorVisible);
            mECS->setInterpretInputs(!editorVisible);
            if (!editorVisible) { mInput->centerMouse(); }
        }

        while (accumulator >= timeStep)
        {
            mInput->updateMouse();
            update(timeStep);
            accumulator -= timeStep;
        }

        const auto alpha = static_cast<float>(accumulator / timeStep);
        render(alpha);
    }

    mWindow->stop();
}

void Engine::update(const float deltaTime) const
{
    mECS->refreshEntities();
    mECS->update(deltaTime);
}

void Engine::render(const float alpha) const
{
    runOnGlThread<void>([this, alpha]
    {
        try
        {
            mECS->render(alpha);
            mSceneRenderer->render();
            mGuiRenderer->render();
        }
        catch (const std::exception& e) { std::cerr << "Error during rendering: " << e.what() << std::endl; }
        mWindow->swapBuffers();
    });
}
