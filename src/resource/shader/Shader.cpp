#include "Shader.h"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

Shader::~Shader()
{
    if (programId != 0)
    {
        glDeleteProgram(programId);
    }
}

Shader::Shader(Shader &&other) noexcept { *this = std::move(other); }

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this == &other) return *this;
    programId = other.programId;
    mLocations = std::move(other.mLocations);
    other.programId = 0;
    return *this;
}

void Shader::use() const { glUseProgram(programId); }

void Shader::setUniform(const std::string &uniform, const GLint value) const
{
    if (const auto location = getLocation(uniform); location >= 0) { glUniform1i(location, value); }
}

void Shader::setUniform(const std::string &uniform, const GLfloat value) const
{
    if (const auto location = getLocation(uniform); location >= 0) { glUniform1f(location, value); }
}

void Shader::setUniform(const std::string &uniform, const glm::vec2 &value) const
{
    if (const auto location = getLocation(uniform); location >= 0) { glUniform2fv(location, 1, glm::value_ptr(value)); }
}

void Shader::setUniform(const std::string &uniform, const glm::vec3 &value) const
{
    if (const auto location = getLocation(uniform); location >= 0) { glUniform3fv(location, 1, glm::value_ptr(value)); }
}

void Shader::setUniform(const std::string &uniform, const glm::vec4 &value) const
{
    if (const auto location = getLocation(uniform); location >= 0) { glUniform4fv(location, 1, glm::value_ptr(value)); }
}

void Shader::setUniformMat3(const std::string &uniform, const glm::mat3 &value) const
{
    if (const auto location = getLocation(uniform); location >= 0)
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void Shader::setUniformMat4(const std::string &uniform, const glm::mat4 &value) const
{
    if (const auto location = getLocation(uniform); location >= 0)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void Shader::setUniformSampler2D(const std::string &name, const int value) const { setUniform(name, value); }

GLint Shader::getLocation(const std::string &uniform) const
{
    if (const auto it = mLocations.find(uniform); it != mLocations.end()) return it->second;
    return -1;
}
