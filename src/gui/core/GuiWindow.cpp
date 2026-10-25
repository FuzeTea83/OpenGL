#include "GuiWindow.h"


#include "core/Engine.h"
#include "core/renderer/GuiRenderer.h"

GuiWindow::GuiWindow(GuiRenderer &editor, const std::string &title, const ImVec2 &position,
                     const ImVec2 &size, const ImGuiWindowFlags flags)
    : mGuiRenderer(editor), mEngine(mGuiRenderer.getEngine()), mTitle(title),
      mPosition(position), mSize(size), mFlags(flags) {}

void GuiWindow::render()
{
    if (!mOpen) { return; }
    if (mFlags & ImGuiWindowFlags_NoMove) { ImGui::SetNextWindowPos(mPosition); }
    if (mFlags & ImGuiWindowFlags_NoResize) { ImGui::SetNextWindowSize(mSize); }
    const float menuBarHeight = ImGui::GetFrameHeight();
    if (ImGui::Begin(mTitle.c_str(), &mOpen, mFlags))
    {
        mHovered = ImGui::IsWindowHovered();
        mFocused = ImGui::IsWindowFocused();
        const ImGuiIO &io = ImGui::GetIO();

        ImGui::PushClipRect(
    ImVec2(0.0f, menuBarHeight),
    ImVec2(io.DisplaySize.x, io.DisplaySize.y),
    true
    );
        ImGui::PopClipRect();
        renderWindowContent();
    }
    ImGui::End();
}

void GuiWindow::open()
{
    mOpen = true;
    const auto &window = mEngine.getWindow();
    onFramebufferSizeChanged(window.getWidth(), window.getHeight());
    onOpen();
}

void GuiWindow::close()
{
    mOpen = false;
    onClose();
}
