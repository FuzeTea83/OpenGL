#ifndef OPENGL_SCENE_LOADER_H
#define OPENGL_SCENE_LOADER_H

#include <nlohmann/json.hpp>

#include "core/renderer/SceneRenderer.h"
#include "gui/window/SceneEditorWindow.h"
#include "utils/Codec.h"
#include "utils/concurrency/CompletableFuture.h"

class Engine;
using json = nlohmann::json;

class SceneManager
{
public:
    explicit SceneManager(Engine &engine);

    bool isLoaded() const { return mSceneLoaded; }

    bool isLoading() const { return mLoading; }

    CompletableFuture<void> loadScene(const std::filesystem::path &scenePath);

    void clearScene() const;

    void exportScene() { exportScene(mScenePath); }

    void exportScene(const std::filesystem::path &scenePath);

    void setScenePath(const std::filesystem::path &scenePath) { mScenePath = scenePath; }

    std::filesystem::path getScenePath() const { return mScenePath; }

    CompletableFuture<void> addModel(const std::filesystem::path &path, const std::string &name, bool generateMipmap);

    CompletableFuture<void> setSkybox(const std::filesystem::path &path, bool emissive);

private:
    CompletableFuture<void> loadSkybox() const;

    CompletableFuture<void> loadModels() const;

    void loadEntities() const;

    void loadEntityNodes(const json &entities, Group *parent) const;

    void loadEntity(const json &entityData, Group *parent, ECS &ecs) const;

    void loadShaderConfig(const json &sceneData) const;

    void exportEntities();

    json exportEntityGroup(const Group *group) const;

    json exportEntity(const std::weak_ptr<Entity> &entity) const;

    void exportShaderConfig();

    Engine &mEngine;
    std::filesystem::path mScenePath{};
    json mSceneData;
    bool mSceneLoaded = false;
    bool mLoading = false;
};


#endif //OPENGL_SCENE_LOADER_H
