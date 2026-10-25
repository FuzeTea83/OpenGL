#ifndef OPENGL_PROPERTY_WINDOW_H
#define OPENGL_PROPERTY_WINDOW_H

#include "SceneEditorWindow.h"
#include "gui/core/GuiWindow.h"

class PropertyWindow : public GuiWindow
{
public:
    explicit PropertyWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void onOpen() override;

private:
    void renderNameForm(Item *item) const;

    static constexpr int bufferSize = 256;
    char *mNameBuffer = new char[bufferSize];
};


#endif //OPENGL_PROPERTY_WINDOW_H
