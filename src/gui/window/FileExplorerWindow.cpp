#include "FileExplorerWindow.h"

#include <iostream>

#include "core/Engine.h"

FileExplorerWindow::FileExplorerWindow(GuiRenderer &editor)
    : GuiWindow(editor, "File Explorer", ImVec2(0, 0), ImVec2(0, 0)) {}

void FileExplorerWindow::renderWindowContent()
{
    if (ImGui::ArrowButton("##Back", ImGuiDir_Left)) { setDirectoryPath(directoryPath.parent_path()); }

    ImGui::SameLine();

    strcpy(pathBuffer, directoryPath.string().c_str());
    ImGui::PushItemWidth(-1);
    if (ImGui::InputText("##Path", pathBuffer, bufferSize,
                         ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        const auto newPath = std::filesystem::path(pathBuffer);
        if (std::filesystem::exists(newPath)) { setDirectoryPath(newPath); }
    }

    renderDirectoryContent();

    if (ImGui::InputText("##SelectedPathEntry", selectedPathEntryBuffer, bufferSize,
                         ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        addPath(directoryPath / selectedPathEntryBuffer);
    }

    ImGui::PopItemWidth();

    if (ImGui::Button("Ok"))
    {
        if (hasFlag(FileExplorerFlags_EnterNewFilename))
        {
            char *path = strtok(selectedPathEntryBuffer, ",");
            while (path != nullptr)
            {
                const std::filesystem::path newPath = directoryPath / path;
                addPath(newPath);
                path = strtok(nullptr, ",");
            }
        }
        selectedPathsCallback(selectedPaths);
        close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) { close(); }
}

void FileExplorerWindow::onFramebufferSizeChanged(int width, int height) {}

void FileExplorerWindow::onOpen()
{
    selectedPaths.clear();
    const Window &window = mEngine.getWindow();
    const int screenWidth = window.getWidth();
    const int screenHeight = window.getHeight();

    const float width = screenWidth / 2.0f;
    const float height = screenHeight / 2.0f;

    setPositionX(width / 2.0f);
    setPositionY(height / 2.0f);
    setSizeX(width);
    setSizeY(height);

    selectedPathEntryBuffer[0] = '\0';
}

void FileExplorerWindow::renderDirectoryContent()
{
    if (!ImGui::BeginChild(
        "Files", ImVec2(0, ImGui::GetContentRegionAvail().y - 2 * ImGui::GetFrameHeightWithSpacing()),
        ImGuiChildFlags_Borders)) { return; }

    std::vector<std::filesystem::directory_entry> entries = getDirectoryContent();
    std::ranges::sort(entries, [](const auto &a, const auto &b)
    {
        return a.path().filename().string() < b.path().filename().string();
    });
    if (entries.size() >= selectionStates.size())
    {
        selectionStates.resize(entries.size(), false);
    }

    for (std::size_t i = 0; i < std::min(entries.size(), selectionStates.size()); ++i)
    {
        const auto &entry = entries[i];
        const bool isFile = std::filesystem::is_regular_file(entry.status());
        bool isSelected = selectionStates[i];
        if (!ImGui::Selectable(((isFile ? "[F] " : "[D] ") + entry.path().filename().string()).c_str(), &isSelected,
                               ImGuiSelectableFlags_AllowDoubleClick)) { continue; }

        if (!hasFlag(FileExplorerFlags_MultipleSelection))
        {
            for (std::size_t j = 0; j < selectionStates.size(); ++j) { selectionStates[j] = false; }
        }
        selectionStates[i] = isSelected;

        if (ImGui::IsMouseDoubleClicked(0))
        {
            if (!isFile) { setDirectoryPath(entry.path()); }
            else if (hasFlag(FileExplorerFlags_AllowSelectionOnDoubleClick))
            {
                addPath(entry.path());
                selectedPathsCallback(selectedPaths);
                close();
            }
        }
        addPath(entry.path());
    }

    ImGui::EndChild();
}

void FileExplorerWindow::recalculateSelectionStates()
{
    selectionStates.clear();
    selectionStates.resize(std::distance(
        std::filesystem::directory_iterator(directoryPath),
        std::filesystem::directory_iterator()));
}

std::vector<std::filesystem::directory_entry> FileExplorerWindow::getDirectoryContent() const
{
    std::vector<std::filesystem::directory_entry> entries;
    try
    {
        for (const auto &entry: std::filesystem::directory_iterator(directoryPath))
        {
            std::string fileName = entry.path().filename().string();
            const bool isFile = std::filesystem::is_regular_file(entry.status());

            if (isFile && hasFlag(FileExplorerFlags_SelectDirectory)) { continue; }

            if (isFile && !typeFilters.empty())
            {
                const std::string extension = entry.path().extension().string();
                if (std::ranges::find(typeFilters, extension) == typeFilters.end()) { continue; }
            }
            entries.push_back(entry);
        }
    }
    catch (const std::filesystem::filesystem_error &e) { std::cerr << "Error: " << e.what() << "\n"; }
    return entries;
}

void FileExplorerWindow::addPath(const std::filesystem::path &path)
{
    const bool isFile = std::filesystem::is_regular_file(path);

    if (isFile && !typeFilters.empty())
    {
        const std::string extension = path.extension().string();
        if (std::ranges::find(typeFilters, extension) == typeFilters.end()) { return; }
    }

    if (hasFlag(FileExplorerFlags_SelectDirectory) && isFile) { return; }

    if (hasFlag(FileExplorerFlags_MultipleSelection))
    {
        if (std::ranges::find(selectedPaths, path) != selectedPaths.end()) { return; }
    }
    else { selectedPaths.clear(); }

    if (!hasFlag(FileExplorerFlags_EnterNewFilename) && !std::filesystem::exists(path)) { return; }

    selectedPaths.push_back(path);
    selectedPathEntryBuffer[0] = '\0';
    for (std::size_t i = 0; i < selectedPaths.size(); ++i)
    {
        if (i > 0) { strcat(selectedPathEntryBuffer, ", "); }
        const std::string fileName = selectedPaths[i].filename().string();
        strcat(selectedPathEntryBuffer, fileName.c_str());
    }
}
