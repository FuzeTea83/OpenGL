#ifndef OPENGL_SHADER_H
#define OPENGL_SHADER_H

#include <string>
#include <unordered_map>
#include <glad/gl.h>
#include <glm/glm.hpp>

using ShaderId = unsigned int;
constexpr ShaderId INVALID_SHADER_ID = 0;

struct Shader
{
    GLuint programId{0};
    std::unordered_map<std::string, GLint> mLocations;

    Shader() = default;

    ~Shader();

    Shader(const Shader &other) = delete;

    Shader &operator=(const Shader &other) = delete;

    Shader(Shader &&other) noexcept;

    Shader &operator=(Shader &&other) noexcept;

    void use() const;

    void setUniform(const std::string &uniform, GLint value) const;

    void setUniform(const std::string &uniform, GLfloat value) const;

    void setUniform(const std::string &uniform, const glm::vec2 &value) const;

    void setUniform(const std::string &uniform, const glm::vec3 &value) const;

    void setUniform(const std::string &uniform, const glm::vec4 &value) const;

    void setUniformMat3(const std::string &uniform, const glm::mat3 &value) const;

    void setUniformMat4(const std::string &uniform, const glm::mat4 &value) const;

    void setUniformSampler2D(const std::string &name, int value) const;

    GLint getLocation(const std::string &uniform) const;
};
#endif //OPENGL_SHADER_H
