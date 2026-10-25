#include "SkyboxImporter.h"

#include "FileExplorerWindow.h"
#include "core/Engine.h"
#include "core/renderer/GuiRenderer.h"


SkyboxImporter::SkyboxImporter(GuiRenderer &editor)
    : GuiWindow(editor, "Model Import", ImVec2(0, 0), ImVec2(0, 0),
                ImGuiWindowFlags_AlwaysAutoResize)
{
    const auto assets = mGuiRenderer.getMenu("Assets");
    if (assets != nullptr) { assets->addMenuItem("Import Skybox", [this] { handleImportSkybox(); }, "CTRL+I"); }
}

void SkyboxImporter::renderWindowContent()
{
    if (ImGui::BeginChild("##Holder", ImVec2(0, ImGui::GetFrameHeightWithSpacing()), false))
    {
        ImGui::Text("Path: %s", mSkyboxPath.string().c_str());
    }
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::Text("Import Options:");
    ImGui::Checkbox("Emissive Texture", &mEmissive);
    ImGui::Separator();

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float halfWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;

    if (ImGui::Button("Import", ImVec2(halfWidth, 0)))
    {
        mEngine.getSceneLoader().setSkybox(mSkyboxPath, mEmissive);
        close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(halfWidth, 0))) { close(); }
}

void SkyboxImporter::onFramebufferSizeChanged(const int width, const int height) {}

void SkyboxImporter::handleImportSkybox()
{
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Import Skybox");
    window.setDirectoryPath("assets/skyboxes");
    window.setTypeFilters({".png", ".hdr"});
    window.setFlags(FileExplorerFlags_AllowSelectionOnDoubleClick);
    window.setSelectedPathsCallback([this](const std::vector<std::filesystem::path> &path)
    {
        setSkyboxPath(path[0]);
        open();
    });
    window.open();
}
