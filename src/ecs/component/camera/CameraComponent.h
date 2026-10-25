#ifndef OPENGL_CAMERA_COMPONENT_H
#define OPENGL_CAMERA_COMPONENT_H

#include "core/Window.h"
#include "ecs/core/Component.h"
#include "utils/Codec.h"

#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

struct CameraVariant : ComponentVariant {};

struct CameraVariantHolder : VariantHolder<CameraVariant>
{
    static std::map<std::string, std::function<void *()> > variants;
};

struct CameraComponent : Component
{
    float fov{45.0f};
    float aspectRatio;
    float nearPlane{NEAR_PLANE};
    float farPlane{FAR_PLANE};
    float yaw{0.0f};
    float pitch{0.0f};
    bool callbackInitialized{false};
    bool setAspectRatioFromWindow{true};
    CameraVariantHolder variant;

    Clonable *clone() const override
    {
        const auto component = new CameraComponent();
        component->fov = fov;
        component->aspectRatio = aspectRatio;
        component->nearPlane = nearPlane;
        component->farPlane = farPlane;
        component->yaw = yaw;
        component->pitch = pitch;
        component->setAspectRatioFromWindow = setAspectRatioFromWindow;
        if (variant.get() != nullptr)
        {
            component->variant.adopt(static_cast<CameraVariant *>(variant.clone()), variant.getVariantName());
        }
        return component;
    }

    void initializeCodec(const Engine &engine) override
    {
        codec.addFloat("Fov", &fov);
        codec.addFloat("Aspect Ratio", &aspectRatio);
        codec.addFloat("Near Plane", &nearPlane);
        codec.addFloat("Far Plane", &farPlane);
        codec.addFloat("Yaw", &yaw);
        codec.addFloat("Pitch", &pitch);
        codec.addOption("Variant", new Codec::Option{
                            .selectedOption = variant.getVariantName(),
                            .options = variant.getVariants(),
                            .onSelected = [this](const std::string &name)
                            {
                                variant.setVariant(name);
                                codec.markDirty();
                            }
                        });
        codec.addBool("Aspect Ratio from Window", &setAspectRatioFromWindow);
        variant.initializeCodec(engine, codec);
    }


    void setAspectRatioCallback(Window &window)
    {
        if (!setAspectRatioFromWindow) { return; }
        aspectRatio = static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight());
        window.addFramebufferSizeCallback([this](const int width, const int height)
        {
            aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        });
    }

    bool hasVariant() const override
    {
        return true;
    }

    void setVariant(const std::string &name) override
    {
        variant.setVariant(name);
    }
};

#endif //OPENGL_CAMERA_COMPONENT_H
