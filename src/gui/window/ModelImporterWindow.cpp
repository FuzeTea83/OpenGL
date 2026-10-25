#include "ModelImporterWindow.h"

#include "FileExplorerWindow.h"
#include "imgui_stdlib.h"
#include "SceneEditorWindow.h"
#include "core/Engine.h"
#include "ecs/component/ModelComponent.h"

ModelImporterWindow::ModelImporterWindow(GuiRenderer &editor)
    : GuiWindow(editor, "Model Import", ImVec2(0, 0), ImVec2(0, 0),
                ImGuiWindowFlags_AlwaysAutoResize)
{
    const auto assets = mGuiRenderer.getMenu("Assets");
    if (assets != nullptr)
    {
        assets->addMenuItem("Import Model", [this] { handleImportModel(); }, "CTRL+I");
    }
}

void ModelImporterWindow::renderWindowContent()
{
    if (ImGui::BeginChild("##Holder", ImVec2(0, ImGui::GetFrameHeightWithSpacing()), false))
    {
        ImGui::Text("Path: %s", mModelPath.string().c_str());
    }
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::Text("Import Options:");
    ImGui::Text("Model Name:");
    ImGui::SameLine();
    ImGui::InputText("##Model", &mModelName);
    ImGui::Checkbox("Generate Mipmaps", &mGenerateMipMaps);
    ImGui::Separator();

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float halfWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;

    if (ImGui::Button("Import", ImVec2(halfWidth, 0)))
    {
        if (!mModelName.empty())
        {
            mEngine.getSceneLoader().addModel(mModelPath, mModelName, mGenerateMipMaps).thenAccept(
                [this] { markSelectedModelComponentDirty(); }, mEngine.getEngineExecutor());
            close();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(halfWidth, 0))) { close(); }
}

void ModelImporterWindow::onFramebufferSizeChanged(const int width, const int height) {}

void ModelImporterWindow::handleImportModel()
{
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Import Model");
    window.setDirectoryPath("assets/models");
    window.setTypeFilters(ASSIMP_SUPPORTED_FORMATS);
    window.setFlags(FileExplorerFlags_AllowSelectionOnDoubleClick);
    window.setSelectedPathsCallback([this](const std::vector<std::filesystem::path> &path)
    {
        setModelPath(path[0]);
        setModelName(path[0].stem().string());
        open();
    });
    window.open();
}

void ModelImporterWindow::markSelectedModelComponentDirty() const
{
    if (!mGuiRenderer.isRegisteredWindow<SceneEditorWindow>()) { return; }
    const auto &window = mGuiRenderer.getWindow<SceneEditorWindow>();
    const auto &item = window.getSelectedItem();
    if (const auto entityItem = dynamic_cast<EntityItem *>(item))
    {
        const auto e = entityItem->entity.lock();
        if (e != nullptr && e->hasComponent<ModelComponent>())
        {
            e->getComponent<ModelComponent>().codec.markDirty();
        }
    }
}
