#include "Window.h"

#include <stdexcept>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "core/Engine.h"
#include "utils/OpenGLUtils.h"
#include "utils/concurrency/CompletableFuture.h"

Window::Window(Engine& engine, const int width, const int height, const char* title) : mEngine(engine),
    mWidth(width),
    mHeight(height)
{
    mEngine.runOnGlThread<void>([this, width, height, title]
    {
        if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW"); }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        mWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);

        if (!mWindow)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(mWindow);
        if (!gladLoadGL([](const char* name) { return glfwGetProcAddress(name); }))
        {
            throw std::runtime_error("Failed to load OpenGL functions");
        }
        glfwSwapInterval(0);
        glViewport(0, 0, mWidth.load(), mHeight.load());
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glfwSetWindowUserPointer(mWindow, this);
        glfwSetWindowMaximizeCallback(mWindow, [](GLFWwindow* window, int maximized)
        {
            assertGlThread();
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            glViewport(0, 0, self->mWidth.load(), self->mHeight.load());            ;
            for (const auto& callback : self->mFramebufferSizeCallbacks) { callback(self->mWidth, self->mHeight); }
        });

        glfwSetFramebufferSizeCallback(mWindow, [](GLFWwindow* window, const int w, const int h)
        {
            assertGlThread();
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            self->mWidth = w;
            self->mHeight = h;
            for (const auto& callback : self->mFramebufferSizeCallbacks) { callback(w, h); }
        });

        glfwSetKeyCallback(mWindow, [](GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
        {
            assertGlThread();
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            for (const auto& callback : self->mKeyCallbacks) { callback(key, scancode, action, mods); }
        });

        glfwSetScrollCallback(mWindow, [](GLFWwindow* window, const double scrollX, const double scrollY)
        {
            assertGlThread();
            const auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            for (const auto& callback : self->mScrollCallbacks) { callback(scrollX, scrollY); }
        });

        glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, const int button, const int action, const int mods)
        {
            assertGlThread();
            const auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            for (const auto& callback : self->mMouseButtonCallbacks) { callback(button, action, mods); }
        });

        setCursorLocked(true);
    });
}

Window::~Window()
{
    mEngine.runOnGlThread<void>([this]
    {
        if (mWindow) { glfwDestroyWindow(mWindow); }
        glfwTerminate();
    });
}

bool Window::shouldClose() const
{
    return mEngine.runOnGlThread<bool>([this]
    {
        return mWindow == nullptr || glfwWindowShouldClose(mWindow);
    });
}

void Window::swapBuffers() const
{
    assertGlThread();
    glfwSwapBuffers(mWindow);
}

void Window::stop() const
{
    if (mWindow)
    {
        mEngine.runOnGlThread<void>([this]
        {
            glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
        });
    }
}

void Window::toggleWireframe() { setWireframe(!mWireframe); }

void Window::setWireframe(const bool wireframe)
{
    mWireframe = wireframe;
    mEngine.runOnGlThread<void>([wireframe]
    {
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    });
}

void Window::setCursorLocked(const bool locked) const
{
    mEngine.runOnGlThread<void>([this, locked]
    {
        glfwSetInputMode(mWindow, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    });
}

void Window::addFramebufferSizeCallback(FramebufferSizeCallback callback)
{
    mFramebufferSizeCallbacks.push_back(std::move(callback));
}

void Window::addKeyCallback(KeyCallback callback) { mKeyCallbacks.push_back(std::move(callback)); }

void Window::addMouseButtonCallback(MouseButtonCallback callback)
{
    mMouseButtonCallbacks.push_back(std::move(callback));
}

void Window::addScrollCallback(ScrollCallback callback) { mScrollCallbacks.push_back(std::move(callback)); }
