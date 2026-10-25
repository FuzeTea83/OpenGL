#ifndef OPENGL_RENDER_MAIN_CAMERA_SYSTEM_H
#define OPENGL_RENDER_MAIN_CAMERA_SYSTEM_H

#include "ecs/core/System.h"
#include "ecs/component/TransformComponent.h"
#include "ecs/component/camera/CameraComponent.h"
#include "ecs/component/camera/MainCameraTag.h"


class RenderMainCameraSystem : public RenderSystem<TransformComponent, CameraComponent, MainCameraTag>
{
public:
    explicit RenderMainCameraSystem(Priority priority);

    void start(const Engine &engine) override { mGlobalUniformsSet = false; }

    void render(const Engine &engine, Entity &entity, float alpha) override;

private:
    bool mGlobalUniformsSet{false};
};


#endif //OPENGL_RENDER_MAIN_CAMERA_SYSTEM_H
