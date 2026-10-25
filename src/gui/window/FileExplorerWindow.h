#ifndef OPENGL_FILE_EXPLORER_H
#define OPENGL_FILE_EXPLORER_H

#include <filesystem>
#include <functional>
#include <mutex>

#include "gui/core/GuiWindow.h"

enum FileExplorerFlags
{
    FileExplorerFlags_SelectDirectory = 1 << 0,             // select directory instead of regular file
    FileExplorerFlags_EnterNewFilename = 1 << 1,            // allow user to enter new filename when selecting regular file
    FileExplorerFlags_MultipleSelection = 1 << 2,           // allow user to select multiple files. this will hide EnterNewFilename
    FileExplorerFlags_AllowSelectionOnDoubleClick = 1 << 3, // allow user to select file on double click
};

using SelectedPathsCallback = std::function<void(const std::vector<std::filesystem::path> &)>;

class FileExplorerWindow : public GuiWindow
{
public:
    explicit FileExplorerWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void onOpen() override;

    std::filesystem::path getDirectoryPath() const { return directoryPath; }

    void setDirectoryPath(const std::filesystem::path &path)
    {
        directoryPath = std::filesystem::absolute(path);
        recalculateSelectionStates();
    }

    bool hasSelectedPaths() const { return !selectedPaths.empty(); }

    std::vector<std::filesystem::path> getSelectedPaths() const { return selectedPaths; }

    void setSelectedPathsCallback(const SelectedPathsCallback &callback)
    {
        selectedPathsCallback = std::move(callback);
    }

    void setTypeFilters(const std::vector<std::string> &filters) { typeFilters = filters; }

    void setFlags(const int newFlags) { flags = newFlags; }

    void setSelectedPaths(const std::vector<std::filesystem::path> &paths) { selectedPaths = paths; }

private:
    bool hasFlag(const FileExplorerFlags flag) const { return flags & flag; }

    void renderDirectoryContent();

    void recalculateSelectionStates();

    std::vector<std::filesystem::directory_entry> getDirectoryContent() const;

    void addPath(const std::filesystem::path &path);

    int flags{0};
    std::filesystem::path directoryPath{};
    std::vector<std::filesystem::path> selectedPaths{};
    static constexpr unsigned int bufferSize{256};
    char pathBuffer[bufferSize]{};
    std::vector<bool> selectionStates{};
    char selectedPathEntryBuffer[bufferSize]{};
    SelectedPathsCallback selectedPathsCallback;
    std::vector<std::string> typeFilters{};
};

#endif //OPENGL_FILE_EXPLORER_H
