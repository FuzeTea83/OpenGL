#include "PropertyWindow.h"

#include <format>
#include <iostream>

#include "SceneEditorWindow.h"
#include "core/Engine.h"
#include "ecs/component/camera/CameraComponent.h"

PropertyWindow::PropertyWindow(GuiRenderer &editor)
    : GuiWindow(editor, "Properties", ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f), ImGuiWindowFlags_None) {}

void PropertyWindow::renderWindowContent()
{
    ECS &ecs = mEngine.getECS();
    const auto &window = mGuiRenderer.getWindow<SceneEditorWindow>();
    const auto &item = window.getSelectedItem();

    renderNameForm(item);

    std::shared_ptr<Entity> entity;
    if (const auto entityItem = dynamic_cast<EntityItem*>(item))
    {
        entity = entityItem->entity.lock();
        if (entity == nullptr) { return; }
    } else
    {
        return;
    }


    ImGui::Separator();

    for (Component *component: entity->getComponents())
    {
        std::string componentName = ecs.getComponentName(component->typeId);
        ImGui::PushID(component);
        ImGui::BeginChild(std::format("Component: {}", componentName).c_str(), ImVec2(0, 0),
                          ImGuiChildFlags_AutoResizeY);
        ImGui::Text("%s", componentName.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) { entity->removeComponent(component->typeId); }

        ImGui::Separator();

        if (component->codec.dirty)
        {
            component->codec.clear();
            component->initializeCodec(mEngine);
            component->codec.clearDirty();
        }
        mGuiRenderer.renderCodec(component->codec);

        ImGui::EndChild();
        ImGui::PopID();
    }

    const std::map<ComponentId, std::string> components = mEngine.getECS().getComponentTypes();

    if (ImGui::Button("Add Component")) { ImGui::OpenPopup("Available Components"); }
    if (ImGui::BeginPopup("Available Components"))
    {
        for (const auto &[id, name]: components)
        {
            if (entity->hasComponent(id) || !ImGui::MenuItem(name.c_str())) { continue; }

            Component &component = entity->addComponent(id);
            component.initializeCodec(mEngine);
            if (const auto cameraComponent = dynamic_cast<CameraComponent *>(&component))
            {
                cameraComponent->setAspectRatioCallback(mEngine.getWindow());
            }
        }
        ImGui::EndPopup();
    }
}


void PropertyWindow::onFramebufferSizeChanged(const int width, const int height)
{
    setPositionX(4.0f / 5.0f * width);
    setPositionY(ImGui::GetMainViewport()->WorkPos.y);
    setSizeX(width / 5.0f);
    setSizeY(ImGui::GetMainViewport()->WorkSize.y);
}

void PropertyWindow::onOpen() { if (!mGuiRenderer.isRegisteredWindow<SceneEditorWindow>()) { close(); } }

void PropertyWindow::renderNameForm(Item *item) const
{
    if (item == nullptr) { return; }
    if (ImGui::BeginTable("form", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);


        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Name:");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);

        strcpy(mNameBuffer, item->getName().c_str());

        if (ImGui::InputText("##Name", mNameBuffer, bufferSize)) { item->setName(mNameBuffer); }

        ImGui::EndTable();
    }
}
