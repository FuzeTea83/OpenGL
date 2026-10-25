#ifndef OPENGL_ANIMATED_MODEL_COMPONENT_H
#define OPENGL_ANIMATED_MODEL_COMPONENT_H

#include "ecs/core/Component.h"
#include "resource/mesh/Mesh.h"

struct ModelAnimator : Component
{
    bool mInitialized{false};
    bool mIsPlaying{true};
    bool mIsLooping{true};
    float mCurrentTime{0};
    float mSpeed{1};
    int mCurrentAnimationIndex{0};
    std::string animation;
    std::vector<Node> runtimeNodes;
    std::vector<glm::mat4> nodeGlobalTransforms;
    std::vector<std::vector<glm::mat4> > skinJointMatrices;
    std::vector<std::string> animationNames;

    Clonable *clone() const override
    {
        return new ModelAnimator(*this);
    }

    void initializeCodec(const Engine &engine) override
    {
        codec.addBool("Play", &mIsPlaying);
        codec.addBool("Loop", &mIsLooping);
        codec.addFloat("Speed", &mSpeed);
        codec.addOption("Animation", new Codec::Option({
                            .selectedOption = animation,
                            .options = animationNames,
                            .onSelected = [&](const std::string &name) { animation = name; }
                        }));
    }
};
#endif //OPENGL_ANIMATED_MODEL_COMPONENT_H
