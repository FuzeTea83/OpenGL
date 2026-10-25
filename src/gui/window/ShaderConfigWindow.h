#ifndef OPENGL_SHADER_CONFIG_WINDOW_H
#define OPENGL_SHADER_CONFIG_WINDOW_H

#include "gui/core/GuiWindow.h"

class ShaderConfigWindow : public GuiWindow
{
public:
    explicit ShaderConfigWindow(GuiRenderer &guiRenderer);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;
};

#endif //OPENGL_SHADER_CONFIG_WINDOW_H
