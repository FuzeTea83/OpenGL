#ifndef OPENGL_MODEL_COMPONENT_H
#define OPENGL_MODEL_COMPONENT_H

#include "core/Engine.h"
#include "ecs/core/Component.h"
#include "resource/mesh/Mesh.h"

struct ModelComponent : Component
{
    MeshDefinition definition{.name = "<no model>", .id = INVALID_MESH_ID};
    glm::vec3 emissiveColor{0.0f};
    float emissiveIntensity{0.0f};
    bool emissivePartsOnly{true};
    bool emissiveOnly{false};
    bool outline{false};

    Clonable *clone() const override
    {
        return new ModelComponent(*this);
    }

    void initializeCodec(const Engine &engine) override
    {
        const auto &meshDefinitions = engine.getMeshManager().getMeshDefinition();

        std::vector<std::string> options;
        for (int i = 0; i < meshDefinitions.size(); ++i)
        {
            const auto &def = meshDefinitions[i];
            options.push_back(def.name);
        }

        if (definition.id == INVALID_MESH_ID && !definition.name.empty())
        {
            for (const auto &def: meshDefinitions)
            {
                if (def.name != definition.name) { continue; }
                definition = def;
                break;
            }
        }

        codec.addOption("Definition", new Codec::Option{
                            .selectedOption = definition.name,
                            .options = options,
                            .onSelected = [&](const std::string &name)
                            {
                                const auto it = std::ranges::find_if(meshDefinitions, [&name](const MeshDefinition &def)
                                {
                                    return def.name == name;
                                });
                                if (it != meshDefinitions.end()) { definition = *it; }
                            }
                        });
        codec.addColor3("Emissive Color", &emissiveColor);
        codec.addFloat("Emissive Intensity", &emissiveIntensity);
        codec.addBool("Emissive Parts Only", &emissivePartsOnly);
        codec.addBool("Emissive Only", &emissiveOnly);
    }
};

#endif //OPENGL_MODEL_COMPONENT_H
