#ifndef OPENGL_SKYBOX_IMPORTER_H
#define OPENGL_SKYBOX_IMPORTER_H

#include <filesystem>

#include "gui/core/GuiWindow.h"


class SkyboxImporter : public GuiWindow
{
public:
    explicit SkyboxImporter(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void setSkyboxPath(const std::filesystem::path &path) { mSkyboxPath = std::filesystem::absolute(path); }

private:
    void handleImportSkybox();

    std::filesystem::path mSkyboxPath;
    bool mEmissive = true;
};

#endif //OPENGL_SKYBOX_IMPORTER_H
