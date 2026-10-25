#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H

#include <functional>
#include <atomic>
#include <vector>
#include <GLFW/glfw3.h>

#include "utils/concurrency/Executor.h"

class Engine;


using FramebufferSizeCallback = std::function<void(int, int)>;
using KeyCallback = std::function<void(int, int, int, int)>;
using MouseButtonCallback = std::function<void(int, int, int)>;
using ScrollCallback = std::function<void(double, double)>;

class Window
{
public:
    explicit Window(Engine &engine, int width = 1280, int height = 720, const char *title = "OpenGL");

    ~Window();

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    bool shouldClose() const;

    void swapBuffers() const;

    void stop() const;

    void toggleWireframe();

    void setWireframe(bool wireframe);

    void setCursorLocked(bool locked) const;

    int getWidth() const { return mWidth.load(); }

    int getHeight() const { return mHeight.load(); }

    float getAspectRatio() const { return static_cast<float>(mWidth) / static_cast<float>(mHeight); }

    GLFWwindow *getNativeHandle() const { return mWindow; }

    void addFramebufferSizeCallback(FramebufferSizeCallback callback);

    void addKeyCallback(KeyCallback callback);

    void addMouseButtonCallback(MouseButtonCallback callback);

    void addScrollCallback(ScrollCallback callback);

private:
    Engine &mEngine;
    GLFWwindow *mWindow{nullptr};
    std::atomic<int> mWidth;
    std::atomic<int> mHeight;
    bool mWireframe{false};
    std::vector<FramebufferSizeCallback> mFramebufferSizeCallbacks;
    std::vector<KeyCallback> mKeyCallbacks;
    std::vector<MouseButtonCallback> mMouseButtonCallbacks;
    std::vector<ScrollCallback> mScrollCallbacks;
};

#endif //OPENGL_WINDOW_H
