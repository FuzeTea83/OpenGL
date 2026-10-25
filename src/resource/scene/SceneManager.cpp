#include "SceneManager.h"

#include <fstream>
#include <iostream>

#include "core/Engine.h"
#include "ecs/component/camera/CameraComponent.h"
#include "gui/window/SceneEditorWindow.h"
#include "resource/mesh/MeshManager.h"

#define ENTITY_ARRAY "entities"
#define ENTITY_NAME "name"
#define ENTITY_VISIBLE "visible"
#define COMPONENT_ARRAY "components"
#define COMPONENT_TYPE "type"
#define COMPONENT_DATA "data"
#define COMPONENT_VARIANT "Variant"
#define MODEL_ARRAY "models"
#define MODEL_NAME "name"
#define MODEL_PATH "path"
#define MODEL_GENERATE_MIPMAP "generateMipmap"
#define SKYBOX "skybox"
#define SKYBOX_PATH "path"
#define SKYBOX_EMISSIVE "emissive"
#define SHADER_CONFIG "shaderConfig"
#define GROUP_CHILDREN "children"

SceneManager::SceneManager(Engine &engine) : mEngine(engine) {}

CompletableFuture<void> SceneManager::loadScene(const std::filesystem::path &scenePath)
{
    if (mLoading) { return CompletableFuture<void>::completed(); }
    clearScene();
    mScenePath = scenePath;
    mSceneLoaded = false;
    mLoading = true;
    std::cout << "Loading scene: " << scenePath << std::endl;

    std::ifstream file(scenePath);
    if (!file)
    {
        mLoading = false;
        std::cout << "Unable to open scene file: " << scenePath.string() << std::endl;
        return CompletableFuture<void>::completed();
    }

    try
    {
        mSceneData = json::parse(file);
    }
    catch (const json::parse_error &e)
    {
        mLoading = false;
        std::cerr << "Failed to parse scene file: " << e.what() << std::endl;
        return CompletableFuture<void>::completed();
    }

    CompletableFuture<void> skyboxFuture = loadSkybox().thenAccept([]
    {
        std::cout << "Skybox loaded successfully." <<
                std::endl;
    }, mEngine.getEngineExecutor());
    CompletableFuture<void> modelsFuture = loadModels();

    CompletableFuture<void> entitiesFuture = modelsFuture.
                                             thenAccept([this] { loadEntities(); }, mEngine.getEngineExecutor()).
                                             thenAccept([]
                                             {
                                                 std::cout << "Entities loaded successfully." << std::endl;
                                             }, mEngine.getEngineExecutor());

    return CompletableFuture<void>::whenAll({skyboxFuture, entitiesFuture}, mEngine.getEngineExecutor())
            .thenAccept([&]
            {
                loadShaderConfig(mSceneData);
                mSceneLoaded = true;
                mLoading = false;
                std::cout << "Scene loaded." << std::endl;
            }, mEngine.getEngineExecutor());
}

void SceneManager::clearScene() const
{
    mEngine.getSceneRenderer().setSkyboxTexture(INVALID_TEXTURE_ID);
    mEngine.getECS().clearEntities();
    mEngine.getGuiRenderer().getWindow<SceneEditorWindow>().clear();
}

void SceneManager::exportScene(const std::filesystem::path &scenePath)
{
    std::cout << "Exporting scene to " << scenePath << std::endl;
    std::ofstream f;
    f.open(scenePath, std::ios::out | std::ios::trunc);

    exportEntities();
    exportShaderConfig();

    f << mSceneData.dump(4);
    f.close();
    std::cout << "Scene exported to " << scenePath << " successfully." << std::endl;
}

CompletableFuture<void> SceneManager::addModel(const std::filesystem::path &path, const std::string &name,
                                               const bool generateMipmap)
{
    return mEngine.getMeshManager().loadMeshAsync(path, name, generateMipmap).thenAccept(
        [this](const MeshDefinition &definition)
        {
            mSceneData[MODEL_ARRAY].push_back({
                {"name", definition.name}, {"path", definition.path}, {"generateMipmap", definition.generateMipmap}
            });
            std::cout << "Added model: " << definition.name << std::endl;
        }, mEngine.getEngineExecutor());
}

CompletableFuture<void> SceneManager::setSkybox(const std::filesystem::path &path, const bool emissive)
{
    json skyboxData = json::object();
    skyboxData[SKYBOX_PATH] = path.string();
    skyboxData[SKYBOX_EMISSIVE] = emissive;
    mSceneData[SKYBOX] = skyboxData;
    return loadSkybox();
}

CompletableFuture<void> SceneManager::loadSkybox() const
{
    if (!mSceneData.contains(SKYBOX))
    {
        std::cout << "Scene file does not contain a skybox. Skipping..." << std::endl;
        return CompletableFuture<void>::completed();
    }

    const json skyboxData = mSceneData[SKYBOX];

    if (!skyboxData.is_object())
    {
        std::cerr << "Scene file does not contain a valid 'skybox' object." << std::endl;
        return CompletableFuture<void>::completed();
    }
    const std::string path = skyboxData[SKYBOX_PATH].get<std::string>();
    const bool emissive = skyboxData.contains(SKYBOX_EMISSIVE) ? skyboxData[SKYBOX_EMISSIVE].get<bool>() : false;
    TextureManager &textureManager = mEngine.getTextureManager();
    return textureManager.loadTextureAsync(path, false, emissive).thenAccept(
        [this](const TextureId textureId) { mEngine.getSceneRenderer().setSkyboxTexture(textureId); },
        mEngine.getGlExecutor());
}

CompletableFuture<void> SceneManager::loadModels() const
{
    if (!mSceneData.contains(MODEL_ARRAY))
    {
        std::cout << "Scene file does not contain models. Skipping..." << std::endl;
        return CompletableFuture<void>::completed();
    }
    if (!mSceneData[MODEL_ARRAY].is_array())
    {
        std::cerr << "Scene file does not contain a valid 'models' array." << std::endl;
        return CompletableFuture<void>::completed();
    }

    return CompletableFuture<void>::runAsync([&]
    {
        MeshManager &meshManager = mEngine.getMeshManager();
        for (const auto &model: mSceneData[MODEL_ARRAY])
        {
            auto path = model[MODEL_PATH].get<std::string>();
            if (!std::filesystem::exists(path))
            {
                std::cerr << "Model file does not exist: " << path << std::endl;
                continue;
            }
            const auto name = model[MODEL_NAME].get<std::string>();
            const auto generateMipmap = model.contains(MODEL_GENERATE_MIPMAP)
                                            ? model[MODEL_GENERATE_MIPMAP].get<bool>()
                                            : true;
            meshManager.loadMesh(path, name, generateMipmap);
        }
        std::cout << "Loaded models successfully." << std::endl;
    }, mEngine.getEngineExecutor());
}

void SceneManager::loadEntities() const
{
    if (!mSceneData.contains(ENTITY_ARRAY))
    {
        std::cout << "Scene file does not contain entities. Skipping..." << std::endl;
        return;
    }
    if (!mSceneData[ENTITY_ARRAY].is_array())
    {
        std::cerr << "Scene file does not contain a valid 'entities' array." << std::endl;
        return;
    }

    std::cout << "Loading entities..." << mSceneData[ENTITY_ARRAY].size() << std::endl;

    loadEntityNodes(mSceneData[ENTITY_ARRAY], mEngine.getGuiRenderer().getWindow<SceneEditorWindow>().getParentGroup());
    std::cout << "Loaded entities successfully." << std::endl;
}

void SceneManager::loadEntityNodes(const json &entities, Group *parent) const
{
    if (!entities.is_array()) { return; }
    ECS &ecs = mEngine.getECS();
    for (const auto &entityData: entities)
    {
        if (!entityData.is_object()) { continue; }

        if (entityData.contains(ENTITY_ARRAY))
        {
            if (!entityData[ENTITY_ARRAY].is_array()) { continue; }
            auto group = std::make_unique<Group>();
            group->visible = entityData.contains(ENTITY_VISIBLE) && entityData[ENTITY_VISIBLE].is_boolean()
                                 ? entityData[ENTITY_VISIBLE].get<bool>()
                                 : true;
            if (entityData.contains(ENTITY_NAME) && entityData[ENTITY_NAME].is_string())
            {
                group->name = entityData[ENTITY_NAME].get<std::string>();
            }
            loadEntityNodes(entityData[ENTITY_ARRAY], group.get());
            parent->items.push_back(std::move(group));
            continue;
        }

        loadEntity(entityData, parent, ecs);
    }
}

void SceneManager::loadEntity(const json &entityData, Group *parent, ECS &ecs) const
{
    if (!entityData.contains(ENTITY_NAME) || !entityData[ENTITY_NAME].is_string()) { return; }

    const auto name = entityData[ENTITY_NAME].get<std::string>();
    const auto entity = ecs.createEntity(name);

    if (entity == nullptr) { return; }

    parent->items.push_back(std::move(std::make_unique<EntityItem>(entity)));

    if (entityData.contains(ENTITY_VISIBLE) && entityData[ENTITY_VISIBLE].is_boolean())
    {
        entity->setVisible(entityData[ENTITY_VISIBLE].get<bool>());
    }

    std::cout << "Loaded entity: " << name << std::endl;

    if (!entityData.contains(COMPONENT_ARRAY) || !entityData[COMPONENT_ARRAY].is_array()) { return;; }

    for (const auto &componentData: entityData[COMPONENT_ARRAY])
    {
        if (!componentData.is_object() || !componentData.contains(COMPONENT_TYPE) || !componentData.
            contains(COMPONENT_DATA)) { continue; }

        const std::string componentType = componentData[COMPONENT_TYPE].get<std::string>();
        const json data = componentData[COMPONENT_DATA];
        const ComponentId componentId = ecs.getComponentIdByName(componentType);

        if (componentId == INVALID_COMPONENT_ID)
        {
            std::cerr << "Component type '" << componentType << "' not registered in ECS." << std::endl;
            continue;
        }

        Component &component = entity->addComponent(componentId);

        if (component.hasVariant() && data.is_object() && data.contains(COMPONENT_VARIANT) && data[COMPONENT_VARIANT].is_string())
        {
            component.setVariant(data[COMPONENT_VARIANT].get<std::string>());
        }
        component.initializeCodec(mEngine);
        component.codec.fromJson(data);

        if (const auto cameraComponent = dynamic_cast<CameraComponent *>(&component))
        {
            cameraComponent->setAspectRatioCallback(mEngine.getWindow());
        }
    }
}

void SceneManager::loadShaderConfig(const json &sceneData) const
{
    if (!sceneData.contains(SHADER_CONFIG))
    {
        std::cout << "Scene file does not contain shader config. Skipping..." << std::endl;
        return;
    }
    if (!sceneData[SHADER_CONFIG].is_object())
    {
        throw std::runtime_error("Scene file does not contain a valid 'shaderConfig' object.");
    }
    mEngine.getSceneRenderer().getShaderConfig().codec.fromJson(sceneData[SHADER_CONFIG]);
}


void SceneManager::exportEntities()
{
    const auto *root = mEngine.getGuiRenderer().getWindow<SceneEditorWindow>().getParentGroup();
    mSceneData[ENTITY_ARRAY] = root->items.empty() ? json::array() : exportEntityGroup(root);
}

json SceneManager::exportEntityGroup(const Group *group) const
{
    json nodes = json::array();
    for (const auto &item: group->items)
    {
        if (const auto entityItem = dynamic_cast<EntityItem *>(item.get()))
        {
            nodes.push_back(exportEntity(entityItem->entity));
        }
        else if (const auto g = dynamic_cast<Group *>(item.get()))
        {
            nodes.push_back({
                {ENTITY_VISIBLE, g->visible},
                {ENTITY_NAME, g->name},
                {ENTITY_ARRAY, exportEntityGroup(g)}
            });
        }
    }

    return nodes;
}

json SceneManager::exportEntity(const std::weak_ptr<Entity> &entity) const
{
    const auto sharedEntity = entity.lock();
    if (sharedEntity == nullptr) { return json::object(); }
    json entityData = json::object();
    ECS &ecs = mEngine.getECS();
    entityData[ENTITY_NAME] = sharedEntity->getName();
    entityData[ENTITY_VISIBLE] = sharedEntity->isVisible();

    entityData[COMPONENT_ARRAY] = json::array();

    for (const auto &component: sharedEntity->getComponents())
    {
        json componentData = json::object();
        component->initializeCodec(mEngine);
        componentData[COMPONENT_TYPE] = ecs.getComponentName(component->typeId);
        componentData[COMPONENT_DATA] = component->codec.toJson();
        entityData[COMPONENT_ARRAY].push_back(componentData);
    }
    return entityData;
}

void SceneManager::exportShaderConfig()
{
    mSceneData[SHADER_CONFIG] = mEngine.getSceneRenderer().getShaderConfig().codec.toJson();
}
