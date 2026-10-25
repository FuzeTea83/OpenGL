#ifndef OPENGL_MAIN_CAMERA_TAG_H
#define OPENGL_MAIN_CAMERA_TAG_H

#include "ecs/core/Component.h"

struct MainCameraTag : Component
{
    Clonable *clone() const override
    {
        return new MainCameraTag();
    }

    void initializeCodec(const Engine &engine) override {}
};


#endif //OPENGL_MAIN_CAMERA_TAG_H
