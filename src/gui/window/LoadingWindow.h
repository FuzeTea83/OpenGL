#ifndef OPENGL_LOADING_WINDOW_H
#define OPENGL_LOADING_WINDOW_H

#include "gui/core/GuiWindow.h"

class LoadingWindow : public GuiWindow
{
public:
    explicit LoadingWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void onOpen() override;

    void onClose() override;
};


#endif //OPENGL_LOADING_WINDOW_H
