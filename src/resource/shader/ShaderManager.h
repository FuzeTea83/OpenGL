#ifndef OPENGL_SHADER_MANAGER_H
#define OPENGL_SHADER_MANAGER_H

#include <filesystem>
#include <unordered_map>
#include <vector>
#include "Shader.h"

class ShaderManager
{
public:
    ShaderManager() = default;

    const Shader &getShader(ShaderId) const;

    ShaderId loadShader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);

private:
    static std::string readFile(const std::filesystem::path &path);

    static GLuint compile(GLenum type, const std::string &source, const std::filesystem::path &path);

    static GLuint compileProgram(const std::string &vertexSource, const std::string &fragmentSource,
                                 const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);

    static void setShaderUniformLocations(Shader &shader);

    static std::size_t hash(std::string_view vertex, std::string_view fragment);

    std::vector<Shader> mShaders;
    std::unordered_map<std::size_t, ShaderId> mLookup;
};
#endif //OPENGL_SHADER_MANAGER_H
