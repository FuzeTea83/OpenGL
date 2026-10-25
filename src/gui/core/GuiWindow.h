#ifndef OPENGL_GUI_WINDOW_H
#define OPENGL_GUI_WINDOW_H

#include <imgui.h>
#include <string>

class Engine;
class GuiRenderer;

using WindowId = unsigned int;

class GuiWindow
{
public:
    GuiWindow(GuiRenderer &editor, const std::string &title, const ImVec2 &position, const ImVec2 &size,
              ImGuiWindowFlags flags = 0);

    virtual ~GuiWindow() = default;

    void render();

    virtual void onFramebufferSizeChanged(int width, int height) = 0;

    virtual void renderWindowContent() = 0;

    virtual void onOpen() {}

    virtual void onClose() {}

    void open();

    void close();

    void disable(const bool disable)
    {
        if (disable) { mFlags |= ImGuiWindowFlags_NoInputs; }
        else { mFlags &= ~ImGuiWindowFlags_NoInputs; }
    }

    void setTitle(const std::string &newTitle) { mTitle = newTitle; }

    std::string &getTitle() { return mTitle; }

    void setPosition(const ImVec2 &newPosition) { mPosition = newPosition; }

    void setPositionX(const float x) { mPosition.x = x; }

    void setPositionY(const float y) { mPosition.y = y; }

    ImVec2 getPosition() const { return mPosition; }

    void setSize(const ImVec2 &newSize) { mSize = newSize; }

    void setSizeX(const float width) { mSize.x = width; }

    void setSizeY(const float height) { mSize.y = height; }

    ImVec2 getSize() const { return mSize; }

    bool isAlwaysVisible() const { return mAlwaysVisible; }

    void setAlwaysVisible(const bool visible) { mAlwaysVisible = visible; }

    bool isOpen() const { return mOpen; }

protected:
    GuiRenderer &mGuiRenderer;
    const Engine &mEngine;

private:
    friend class GuiRenderer;

    bool mAlwaysVisible = false;
    bool mOpen = false;
    bool mHovered = false;
    bool mFocused = false;
    std::string mTitle;
    ImVec2 mPosition{0.0f, 0.0f};
    ImVec2 mSize{0.0f, 0.0f};
    ImGuiWindowFlags mFlags;
};

inline WindowId getNextWindowTypeId()
{
    static WindowId lastId = 0;
    return lastId++;
}

template<typename T>
WindowId getWindowTypeId() noexcept
{
    static_assert(std::is_base_of_v<GuiWindow, T>, "T must derive from GuiWindow");
    static const WindowId id = getNextWindowTypeId();
    return id;
}

#endif //OPENGL_GUI_WINDOW_H
