#include "Input.h"

#include <GLFW/glfw3.h>

#include "core/Engine.h"
#include "core/Window.h"

Input::Input(Engine &engine, Window &window) : mEngine(engine), mWindow(window)
{
    registerBinding({GLFW_KEY_W}, Action::MoveForward);
    registerBinding({GLFW_KEY_D}, Action::MoveRight);
    registerBinding({GLFW_KEY_S}, Action::MoveBackward);
    registerBinding({GLFW_KEY_A}, Action::MoveLeft);
    registerBinding({GLFW_KEY_E}, Action::MoveUp);
    registerBinding({GLFW_KEY_Q}, Action::MoveDown);
    registerBinding({GLFW_KEY_F11}, Action::ToggleWireFrame);
    registerBinding({GLFW_KEY_C}, Action::ChangeAnimationRunningState);
    registerBinding({GLFW_KEY_X}, Action::AllowCameraMovement);
    registerBinding({GLFW_KEY_LEFT_ALT}, Action::ToggleMouseLock);
    registerBinding({GLFW_KEY_LEFT_ALT}, Action::ToggleMouseLock);
    registerBinding({GLFW_KEY_F1}, Action::ToggleEditor);
    registerBinding({GLFW_KEY_LEFT_CONTROL, GLFW_KEY_C}, Action::Copy);
    registerBinding({GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_C}, Action::Copy);
    registerBinding({GLFW_KEY_LEFT_CONTROL, GLFW_KEY_V}, Action::Paste);
    registerBinding({GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_V}, Action::Paste);

    mWindow.addKeyCallback([this](const int key, const int scancode, const int action, const int mods)
    {
        if (action == GLFW_PRESS)
        {
            mKeys[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mKeys[key] = false;
        }
    });

    mWindow.addScrollCallback([this](const double scrollX, const double scrollY)
    {
        mScrollX += static_cast<float>(scrollX);
        mScrollY += static_cast<float>(scrollY);
    });
}


void Input::updateMouse()
{
    mEngine.runOnGlThread<void>([this]
    {
        mScrollX = 0;
        mScrollY = 0;
        double mouseX;
        double mouseY;
        glfwGetCursorPos(mWindow.getNativeHandle(), &mouseX, &mouseY);
        mMouseX.store(mouseX);
        mMouseY.store(mouseY);
        if (mMouseLocked)
        {
            glfwSetCursorPos(mWindow.getNativeHandle(), static_cast<float>(mWindow.getWidth()) / 2.0f,
                             static_cast<float>(mWindow.getHeight()) / 2.0f);
        }
    });
}

double Input::getDeltaX() const { return static_cast<float>(mWindow.getWidth()) / 2.0f - mMouseX.load(); }

double Input::getDeltaY() const { return static_cast<float>(mWindow.getHeight()) / 2.0f - mMouseY.load(); }

void Input::lockMouse(const bool lock)
{
    mMouseLocked = lock;
    mWindow.setCursorLocked(mMouseLocked);
}

void Input::centerMouse() const
{
    mEngine.runOnGlThread<void>([this]
    {
        glfwSetCursorPos(mWindow.getNativeHandle(), static_cast<float>(mWindow.getWidth()) / 2.0f,
                         static_cast<float>(mWindow.getHeight()) / 2.0f);
    });
}

bool Input::isPressed(const Action action, const std::array<bool, 348> &keyArray) const {
    const std::vector<std::vector<unsigned int>> keyBindings = mBindings.at(action);
    for (auto keys : keyBindings)
    {
        bool pressed = true;
        for (const auto key : keys)
        {
            if (!keyArray[key])
            {
                pressed = false;
                break;
            }
        }
        if (pressed)
        {
            return true;
        }
    }

    return false;
}
