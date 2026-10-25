#include "ShaderConfigWindow.h"

#include <fstream>

#include "core/Engine.h"

ShaderConfigWindow::ShaderConfigWindow(GuiRenderer &guiRenderer)
    : GuiWindow(guiRenderer, "Shader Config", ImVec2(0, ImGui::GetMainViewport()->WorkPos.y), ImVec2(0, 0),
                ImGuiWindowFlags_None) {}

void ShaderConfigWindow::renderWindowContent()
{
    mGuiRenderer.renderCodec(mEngine.getSceneRenderer().getShaderConfig().codec);
}

void ShaderConfigWindow::onFramebufferSizeChanged(const int width, const int height)
{
    setPositionY(ImGui::GetMainViewport()->WorkPos.y);
    setSizeX(width / 5);
    setSizeY(ImGui::GetMainViewport()->WorkSize.y);
}
