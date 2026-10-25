#ifndef OPENGL_FPS_CAMERA_VARIANT_H
#define OPENGL_FPS_CAMERA_VARIANT_H

#include "ecs/component/camera/CameraComponent.h"

struct FpsCameraVariant : CameraVariant
{
    glm::vec3 offset{0.0f};

    Clonable *clone() const override
    {
        return new FpsCameraVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Offset", &offset);
    }
};

REGISTER_VARIANT(CameraVariantHolder, "Fps Camera", FpsCameraVariant);


#endif //OPENGL_FPS_CAMERA_VARIANT_H
