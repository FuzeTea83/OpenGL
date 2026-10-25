#ifndef OPENGL_TEXTURE_H
#define OPENGL_TEXTURE_H

#include <glad/gl.h>

using TextureId = unsigned int;
constexpr TextureId INVALID_TEXTURE_ID = 0;

struct Texture
{
    GLuint glId{0};
    bool cubeMap{false};
    bool transparent{false};
};

struct StbImage
{
    void *data{nullptr};
    int width{0}, height{0}, channels{0};
    GLint internalFormat{GL_RGB};
    GLenum dataFormat{GL_RGB};
    bool hdr{false};
    bool isTransparent{false};
};

#endif //OPENGL_TEXTURE_H
