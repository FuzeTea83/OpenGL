#ifndef OPENGL_INPUT_H
#define OPENGL_INPUT_H

#include <array>
#include <atomic>
#include <queue>
#include <unordered_map>

#include "GLFW/glfw3.h"

class Engine;
class Window;

class Input
{
public:
    enum class Action
    {
        MoveForward, MoveBackward, MoveLeft, MoveRight, MoveUp, MoveDown,
        ChangeAnimationRunningState, AllowCameraMovement,
        ToggleWireFrame, ToggleMouseLock, ToggleEditor,
        Copy, Paste,
        ACTION_COUNT
    };

    explicit Input(Engine& engine, Window& window);

    void setSensitivity(const double sensitivity) { mSensitivity = sensitivity; }

    double getSensitivity() const { return mSensitivity; }

    void updateKeyboard() { mPreviousKeys = mKeys; }

    void updateMouse();

    bool isPressed(const Action action) const
    {
        return isPressed(action, mKeys);
    }

    bool isPressedThisFrame(const Action action) const
    {
        return !isPressed(action, mPreviousKeys) && isPressed(action, mKeys);
    }

    double getMouseX() const { return mMouseX.load(); }

    double getMouseY() const { return mMouseY.load(); }

    double getDeltaX() const;

    double getDeltaY() const;

    double getScrollX() const { return mScrollX.load(); }

    double getScrollY() const { return mScrollY.load(); }

    void toggleMouseLock()
    {
        mMouseLocked = !mMouseLocked;
        lockMouse(mMouseLocked);
    }

    void lockMouse(bool lock);

    bool isMouseLocked() const { return mMouseLocked; }

    void centerMouse() const;

    void registerBinding(const std::vector<unsigned int>& keys, Action action)
    {
        mBindings[action].push_back(keys);
    }

private:
    bool isPressed(Action action, const std::array<bool, GLFW_KEY_LAST> &keyArray) const;

    Engine& mEngine;
    Window& mWindow;
    double mSensitivity = 0.1;
    std::atomic<double> mMouseX = 0.0;
    std::atomic<double> mMouseY = 0.0;
    std::atomic<double> mDeltaX = 0.0;
    std::atomic<double> mDeltaY = 0.0;
    std::atomic<double> mScrollX = 0.0;
    std::atomic<double> mScrollY = 0.0;
    bool mMouseLocked = true;
    std::queue<unsigned int> keyEvents;
    std::array<bool, GLFW_KEY_LAST> mKeys = {};
    std::array<bool, GLFW_KEY_LAST> mPreviousKeys = {};
    std::unordered_map<Action, std::vector<std::vector<unsigned int>>> mBindings = {};
};

#endif //OPENGL_INPUT_H
