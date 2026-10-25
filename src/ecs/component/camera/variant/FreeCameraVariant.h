#ifndef OPENGL_FREE_CAMERA_VARIANT_H
#define OPENGL_FREE_CAMERA_VARIANT_H

#include "ecs/component/TransformComponent.h"
#include "ecs/component/camera/CameraComponent.h"

struct FreeCameraVariant : CameraVariant
{
    Transform transform;

    Clonable *clone() const override
    {
        return new FreeCameraVariant(*this);
    }

    void initializeCodec(const Engine &engine, Codec &codec) override
    {
        codec.addVec3("Position", &transform.position);
        codec.addQuaternion("Rotation", &transform.rotation);
        codec.addVec3("Scale", &transform.scale);
    }

};

REGISTER_VARIANT(CameraVariantHolder, "Free Camera", FreeCameraVariant);

#endif //OPENGL_FREE_CAMERA_VARIANT_H
