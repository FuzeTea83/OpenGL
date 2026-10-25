#include "ShaderManager.h"

#include <fstream>
#include <stdexcept>

#include "utils/OpenGLUtils.h"

const Shader &ShaderManager::getShader(const ShaderId id) const
{
    if (id == 0 || id > mShaders.size()) throw std::runtime_error("Invalid shader ID");
    return mShaders[id - 1];
}

ShaderId ShaderManager::loadShader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath)
{
    assertGlThread();
    const std::string vertexSource = readFile(vertexPath.string());
    const std::string fragmentSource = readFile(fragmentPath.string());

    std::size_t h = hash(std::string_view(vertexSource), std::string_view(fragmentSource));

    if (const auto it = mLookup.find(h); it != mLookup.end()) { return it->second; }

    const GLuint programId = compileProgram(vertexSource, fragmentSource, vertexPath, fragmentPath);

    Shader shader;
    shader.programId = programId;
    setShaderUniformLocations(shader);

    const auto id = static_cast<ShaderId>(mShaders.size() + 1);
    mShaders.emplace_back(std::move(shader));
    mLookup.emplace(h, id);
    return id;
}

std::string ShaderManager::readFile(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Unable to read shader: " + path.string());
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint ShaderManager::compile(const GLenum type, const std::string &source, const std::filesystem::path &path)
{
    const GLuint shader = glCreateShader(type);
    const char *text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + path.string() + "\n" + std::string(log));
    }
    return shader;
}

GLuint ShaderManager::compileProgram(const std::string &vertexSource, const std::string &fragmentSource,
                                     const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath)
{
    const GLuint vs = compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok;
    glGetProgramiv(programId, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        glGetProgramInfoLog(programId, sizeof(log), nullptr, log);
        glDeleteProgram(programId);
        programId = 0;
        throw std::runtime_error(log);
    }
    return programId;
}

void ShaderManager::setShaderUniformLocations(Shader &shader)
{
    GLint count = 0;
    glGetProgramiv(shader.programId, GL_ACTIVE_UNIFORMS, &count);

    if (count == 0) return;

    char name[256];
    GLsizei len;
    GLint size;
    GLenum type;

    for (GLuint i = 0; i < count; i++)
    {
        glGetActiveUniform(shader.programId, i, sizeof(name), &len, &size, &type, name);
        const GLint location = glGetUniformLocation(shader.programId, name);
        if (location >= 0) { shader.mLocations[name] = location; }
    }
}

static void hashCombine(std::size_t &seed, const std::size_t v) { seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); }

std::size_t ShaderManager::hash(const std::string_view vertex, const std::string_view fragment)
{
    std::size_t h = std::hash<std::string_view>{}(vertex);
    hashCombine(h, std::hash<std::string_view>{}(fragment));
    return h;
}
