#ifndef OPENGL_MODEL_IMPORTER_WINDOW_H
#define OPENGL_MODEL_IMPORTER_WINDOW_H

#include <filesystem>

#include "gui/core/GuiWindow.h"

class ModelImporterWindow : public GuiWindow
{
public:
    explicit ModelImporterWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void setModelPath(const std::filesystem::path &path) { mModelPath = std::filesystem::absolute(path); }

    void setModelName(const std::string &string) { mModelName = string; }

private:
    void handleImportModel();

    void markSelectedModelComponentDirty() const;

    std::filesystem::path mModelPath;
    std::string mModelName;
    bool mGenerateMipMaps = false;
};

#endif //OPENGL_MODEL_IMPORTER_WINDOW_H
