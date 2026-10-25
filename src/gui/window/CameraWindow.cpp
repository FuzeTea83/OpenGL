#include "CameraWindow.h"

#include "core/Engine.h"
#include "ecs/component/camera/CameraComponent.h"
#include "ecs/component/camera/MainCameraTag.h"

CameraWindow::CameraWindow(GuiRenderer &editor)
    : GuiWindow(editor, "Camera", ImVec2(0, 0), ImVec2(0, 0), ImGuiWindowFlags_AlwaysAutoResize) {}

void CameraWindow::renderWindowContent()
{
    if (mCamera == nullptr)
    {
        const auto tags = mEngine.getECS().query<MainCameraTag>();
        if (!tags.empty()) { mCamera = tags.front(); }
    }

    auto cameras = mEngine.getECS().query<CameraComponent>();

    if (!ImGui::BeginCombo(std::string("##Cameras").c_str(), getName().c_str())) { return; }

    std::ranges::sort(cameras, [](const auto *a, const auto *b)
    {
        if (a->isVisible() != b->isVisible()) { return a->isVisible() > b->isVisible(); }
        return a->getName() < b->getName();
    });

    for (auto *entity: cameras)
    {
        ImGui::PushID(entity);
        const bool isSelected = (entity == mCamera);
        const bool visible = entity->isVisible();
        if (!visible) { ImGui::BeginDisabled(); }
        if (ImGui::Selectable(entity->getName().c_str(), isSelected)) { setCamera(entity); }
        if (isSelected) { ImGui::SetItemDefaultFocus(); }
        if (!visible) { ImGui::EndDisabled(); }
        ImGui::PopID();
    }

    ImGui::EndCombo();
}

void CameraWindow::onFramebufferSizeChanged(int width, int height) {}

void CameraWindow::setCamera(Entity *entity)
{
    mCamera->removeComponent<MainCameraTag>();
    mCamera = entity;
    if (mCamera != nullptr && !mCamera->hasComponent<MainCameraTag>()) { mCamera->addComponent<MainCameraTag>(); }
}

std::string CameraWindow::getName() const { return mCamera != nullptr ? mCamera->getName() : "<no camera>"; }
