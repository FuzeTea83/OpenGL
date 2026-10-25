#ifndef OPENGL_CODEC
#define OPENGL_CODEC

#include <map>
#include <string>
#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Codec
{
    struct Option
    {
        std::string selectedOption;
        std::vector<std::string> options;
        std::function<void(const std::string &)> onSelected;
    };

    struct CustomJson
    {
        std::function<json()> toJson;
        std::function<void(const json &)> fromJson;
    };

    using property = std::variant<bool *, int *, float *, glm::vec2 *, glm::vec3 *, glm::vec4 *, glm::quat *,
        std::string *, Option *, std::vector<std::string> *, Codec *, CustomJson *>;

    enum class Type
    {
        BOOL, INT, FLOAT, VEC2, VEC3, VEC4, QUATERNION, STRING, COLOR3, COLOR4, OPTION, CODEC, CUSTOM_JSON
    };

    Codec &addBool(const std::string &name, bool *value)
    {
        properties[name] = {Type::BOOL, value};
        return *this;
    }

    Codec &addInt(const std::string &name, int *value)
    {
        properties[name] = {Type::INT, value};
        return *this;
    }

    Codec &addFloat(const std::string &name, float *value)
    {
        properties[name] = {Type::FLOAT, value};
        return *this;
    }

    Codec &addVec2(const std::string &name, glm::vec2 *value)
    {
        properties[name] = {Type::VEC2, value};
        return *this;
    }

    Codec &addVec3(const std::string &name, glm::vec3 *value)
    {
        properties[name] = {Type::VEC3, value};
        return *this;
    }

    Codec &addVec4(const std::string &name, glm::vec4 *value)
    {
        properties[name] = {Type::VEC4, value};
        return *this;
    }

    Codec &addQuaternion(const std::string &name, glm::quat *value)
    {
        properties[name] = {Type::QUATERNION, value};
        return *this;
    }

    Codec &addString(const std::string &name, std::string *value)
    {
        properties[name] = {Type::STRING, value};
        return *this;
    }

    Codec &addColor3(const std::string &name, glm::vec3 *value)
    {
        properties[name] = {Type::COLOR3, value};
        return *this;
    }

    Codec &addColor4(const std::string &name, glm::vec4 *value)
    {
        properties[name] = {Type::COLOR4, value};
        return *this;
    }

    Codec &addOption(const std::string &name, Option *value)
    {
        properties[name] = {Type::OPTION, value};
        return *this;
    }


    Codec &addCodec(const std::string &name, Codec *codec)
    {
        properties[name] = {Type::CODEC, codec};
        return *this;
    }

    Codec &addCustomJson(const std::string &name, CustomJson *custom)
    {
        properties[name] = {Type::CUSTOM_JSON, custom};
        return *this;
    }

    Option *getOption(const std::string &name) const { return std::get<Option *>(properties.at(name).second); }

    json toJson() const
    {
        json data = json::object();

        for (auto &[name, prop]: properties)
        {
            switch (prop.first)
            {
                case Type::BOOL: data[name] = *std::get<bool *>(prop.second);
                    break;
                case Type::INT: data[name] = *std::get<int *>(prop.second);
                    break;
                case Type::FLOAT: data[name] = *std::get<float *>(prop.second);
                    break;
                case Type::VEC2:
                {
                    data[name] = {
                        std::get<glm::vec2 *>(prop.second)->x, std::get<glm::vec2 *>(prop.second)->y
                    };
                    break;
                }
                case Type::VEC3:
                {
                    data[name] = {
                        std::get<glm::vec3 *>(prop.second)->x, std::get<glm::vec3 *>(prop.second)->y,
                        std::get<glm::vec3 *>(prop.second)->z
                    };
                    break;
                }
                case Type::VEC4:
                {
                    data[name] = {
                        std::get<glm::vec4 *>(prop.second)->x, std::get<glm::vec4 *>(prop.second)->y,
                        std::get<glm::vec4 *>(prop.second)->z, std::get<glm::vec4 *>(prop.second)->w
                    };
                    break;
                }
                case Type::QUATERNION:
                {
                    data[name] = {
                        std::get<glm::quat *>(prop.second)->x, std::get<glm::quat *>(prop.second)->y,
                        std::get<glm::quat *>(prop.second)->z, std::get<glm::quat *>(prop.second)->w
                    };
                    break;
                }
                case Type::STRING:
                {
                    data[name] = *std::get<std::string *>(prop.second);
                    break;
                }
                case Type::COLOR3:
                {
                    data[name] = {
                        std::get<glm::vec3 *>(prop.second)->x, std::get<glm::vec3 *>(prop.second)->y,
                        std::get<glm::vec3 *>(prop.second)->z
                    };
                    break;
                }
                case Type::COLOR4:
                {
                    data[name] = {
                        std::get<glm::vec4 *>(prop.second)->x, std::get<glm::vec4 *>(prop.second)->y,
                        std::get<glm::vec4 *>(prop.second)->z, std::get<glm::vec4 *>(prop.second)->w
                    };
                    break;
                }
                case Type::OPTION: data[name] = std::get<Option *>(prop.second)->selectedOption;
                    break;
                case Type::CODEC: data[name] = std::get<Codec *>(prop.second)->toJson();
                    break;
                case Type::CUSTOM_JSON:
                {
                    auto *custom = std::get<CustomJson *>(prop.second);
                    if (custom && custom->toJson)
                    {
                        data[name] = custom->toJson();
                    }
                    break;
                }
            }
        }

        return data;
    }


    void fromJson(const json &data)
    {
        for (auto &[name, prop]: properties)
        {
            if (!data.contains(name)) { continue; }

            switch (prop.first)
            {
                case Type::BOOL: *std::get<bool *>(prop.second) = data[name].get<bool>();
                    break;
                case Type::INT: *std::get<int *>(prop.second) = data[name].get<int>();
                    break;
                case Type::FLOAT: *std::get<float *>(prop.second) = data[name].get<float>();
                    break;
                case Type::VEC2:
                {
                    const auto &vec2Data = data[name];
                    *std::get<glm::vec2 *>(prop.second) = glm::vec2(vec2Data[0], vec2Data[1]);
                    break;
                }
                case Type::VEC3:
                {
                    const auto &vec3Data = data[name];
                    *std::get<glm::vec3 *>(prop.second) = glm::vec3(vec3Data[0], vec3Data[1], vec3Data[2]);
                    break;
                }
                case Type::VEC4:
                {
                    const auto &vec4Data = data[name];
                    *std::get<glm::vec4 *>(prop.second) = glm::vec4(vec4Data[0], vec4Data[1], vec4Data[2],
                                                                    vec4Data[3]);
                    break;
                }
                case Type::QUATERNION:
                {
                    const auto &quatData = data[name];
                    *std::get<glm::quat *>(prop.second) = glm::quat(quatData[3], quatData[0], quatData[1],
                                                                    quatData[2]);
                    break;
                }
                case Type::STRING: *std::get<std::string *>(prop.second) = data[name].get<std::string>();
                    break;
                case Type::COLOR3:
                {
                    const auto &colorData = data[name];
                    *std::get<glm::vec3 *>(prop.second) = glm::vec3(colorData[0], colorData[1], colorData[2]);
                    break;
                }
                case Type::COLOR4:
                {
                    const auto &colorData = data[name];
                    *std::get<glm::vec4 *>(prop.second) = glm::vec4(colorData[0], colorData[1], colorData[2],
                                                                    colorData[3]);
                    break;
                }
                case Type::OPTION:
                {
                    const auto option = std::get<Option *>(prop.second);
                    option->selectedOption = data[name].get<std::string>();
                    option->onSelected(option->selectedOption);
                    break;
                }
                case Type::CODEC:
                {
                    std::get<Codec *>(prop.second)->fromJson(data[name]);
                    break;
                }
                case Type::CUSTOM_JSON:
                {
                    auto *custom = std::get<CustomJson *>(prop.second);
                    if (custom && custom->fromJson)
                    {
                        custom->fromJson(data[name]);
                    }
                    break;
                }
            }
        }
    }

    void reset() { properties.clear(); }

    void markDirty() { dirty = true; }

    void clearDirty() { dirty = false; }

    void clear() { properties.clear(); }

    std::map<std::string, std::pair<Type, property> > properties;

    bool dirty = false;
};

#endif //OPENGL_CODEC
