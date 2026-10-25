#include "LoadingWindow.h"

#include <cmath>

#include "core/renderer/GuiRenderer.h"

LoadingWindow::LoadingWindow(GuiRenderer &editor)
    : GuiWindow(editor, "Loading", ImVec2(0, 0), ImVec2(200, 0),
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize) {}

void LoadingWindow::renderWindowContent()
{
    static float indeterminate = 0.0f;
    indeterminate = std::fmodf(indeterminate + ImGui::GetIO().DeltaTime * 1.5f, 1.0f);
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 12.0f),
                                              ImGui::GetColorU32(ImGuiCol_FrameBg));
    const float bar_w = size.x * 0.4f;
    const float x = (size.x - bar_w) * indeterminate;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x + x, pos.y), ImVec2(pos.x + x + bar_w, pos.y + 12.0f),
                                              ImGui::GetColorU32(ImGuiCol_PlotHistogram));
    ImGui::Dummy(ImVec2(size.x, 12.0f));
}

void LoadingWindow::onFramebufferSizeChanged(int width, int height) {}

void LoadingWindow::onOpen() { mGuiRenderer.disableAllWindowsExcept(*this); }

void LoadingWindow::onClose() { mGuiRenderer.enableAllWindows(); }
