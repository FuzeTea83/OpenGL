#ifndef OPENGL_CAMERA_WINDOW_H
#define OPENGL_CAMERA_WINDOW_H

#include "ecs/core/Entity.h"
#include "gui/core/GuiWindow.h"

class CameraWindow : public GuiWindow {
public:
    explicit CameraWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

private:
    void setCamera(Entity *entity);

    std::string getName() const;

    Entity* mCamera = nullptr;
};

#endif //OPENGL_CAMERA_WINDOW_H
