#ifndef OPENGL_OPENGL_UTILS_H
#define OPENGL_OPENGL_UTILS_H

#include <iostream>
#include <GLFW/glfw3.h>

inline void PrintGLError(const std::string& function)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) { std::cerr << "OpenGL Error in " << function << ": " << error << std::endl; }
}

inline bool isGlThread() {
    return glfwGetCurrentContext() != nullptr;
}

inline void assertGlThread()
{
    if (glfwGetCurrentContext() == nullptr)
    {
        throw std::runtime_error("No OpenGL context is current on the calling thread.");
    }
}
#endif //OPENGL_OPENGL_UTILS_H
