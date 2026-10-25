#include "SceneEditorWindow.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "FileExplorerWindow.h"
#include "LoadingWindow.h"
#include "PropertyWindow.h"
#include "core/Engine.h"
#include "ecs/component/ModelComponent.h"
#include "ecs/core/Entity.h"
#include "utils/IconsMaterialSymbols.h"

namespace
{
struct SceneItemPayload
{
    Item *item;
    Group *parent;
};
}

void EntityItem::render(SceneEditorWindow &window, Group &parent)
{
    const auto e = entity.lock();
    if (e == nullptr) { return; }
    ImGui::PushID(this);

    const bool isVisible = e->isVisible();
    const char *icon = isVisible ? ICON_MS_VISIBILITY : ICON_MS_VISIBILITY_OFF;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (window.mSelectedItem == this) { flags |= ImGuiTreeNodeFlags_Selected; }

    ImGui::TreeNodeEx(e->getName().c_str(), flags);
    const ImVec2 rowMin = ImGui::GetItemRectMin();
    const ImVec2 rowMax = ImGui::GetItemRectMax();
    const float rowCenterY = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f;
    const ImGuiStyle &style = ImGui::GetStyle();
    const float spacing = style.ItemSpacing.x;
    const float w1 = ImGui::CalcTextSize(icon).x + style.FramePadding.x * 2.0f;
    const float w2 = ImGui::CalcTextSize("X").x + style.FramePadding.x * 2.0f;
    const float totalWidth = w1 + w2 + spacing;
    const float buttonsStartX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - totalWidth;
    if (ImGui::IsItemClicked())
    {
        if (window.mSelectedItem != this)
        {
            window.mSelectedEntityChanged = true;
            window.mSelectedItem = this;
        }
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        const SceneItemPayload payload{.item = this, .parent = &parent};
        ImGui::SetDragDropPayload("SCENE_ENTITY_ITEM", &payload, sizeof(payload));
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY_ITEM",
                                                                       ImGuiDragDropFlags_AcceptNoDrawDefaultRect |
                                                                       ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
            if (payload->DataSize == sizeof(SceneItemPayload))
            {
                const auto &data = *static_cast<const SceneItemPayload *>(payload->Data);
                if (data.item != this)
                {
                    const bool insertAfter = ImGui::GetMousePos().y > rowCenterY;
                    const float markerY = insertAfter ? rowMax.y : rowMin.y;
                    const float lineStartX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(lineStartX, markerY), ImVec2(buttonsStartX, markerY),
                                                        ImGui::GetColorU32(ImGuiCol_DragDropTarget), 3.0f);
                    if (payload->IsDelivery())
                    {
                        window.queueItemMove(data.item, data.parent, &parent, this, insertAfter);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    const ImVec2 rowCursor = ImGui::GetCursorScreenPos();

    const float rowHeight = rowMax.y - rowMin.y;
    const float rowGapWidth = std::max(0.0f, buttonsStartX - rowMax.x);
    ImGui::SetCursorScreenPos(ImVec2(rowMax.x, rowMin.y));
    ImGui::InvisibleButton("EntityRowGap", ImVec2(rowGapWidth, rowHeight));
    const bool hoveredInRowGap = ImGui::IsItemHovered();
    const ImVec2 mousePos = ImGui::GetMousePos();
    if (hoveredInRowGap && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        window.mSelectedItem != this)
    {
        window.mSelectedEntityChanged = true;
        window.mSelectedItem = this;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        const SceneItemPayload payload{.item = this, .parent = &parent};
        ImGui::SetDragDropPayload("SCENE_ENTITY_ITEM", &payload, sizeof(payload));
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY_ITEM",
                                                                       ImGuiDragDropFlags_AcceptNoDrawDefaultRect |
                                                                       ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
            if (payload->DataSize == sizeof(SceneItemPayload))
            {
                const auto &data = *static_cast<const SceneItemPayload *>(payload->Data);
                if (data.item != this)
                {
                    const bool insertAfter = mousePos.y > rowCenterY;
                    const float markerY = insertAfter ? rowMax.y : rowMin.y;
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(rowMin.x, markerY), ImVec2(buttonsStartX, markerY),
                                                        ImGui::GetColorU32(ImGuiCol_DragDropTarget), 3.0f);
                    if (payload->IsDelivery())
                    {
                        window.queueItemMove(data.item, data.parent, &parent, this, insertAfter);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SetCursorScreenPos(ImVec2(buttonsStartX, rowMin.y));
    if (ImGui::SmallButton(icon)) { e->setVisible(!isVisible); }

    ImGui::SameLine();
    if (ImGui::SmallButton("X")) { e->remove(); }

    ImGui::SetCursorScreenPos(ImVec2(rowCursor.x, rowMax.y));

    ImGui::PopID();
}

std::string EntityItem::getName()
{
    if (const auto e = entity.lock()) { return e->getName(); }
    return "";
}

void EntityItem::setName(const std::string &name) { if (const auto e = entity.lock()) { return e->setName(name); } }

void Group::render(SceneEditorWindow &window, Group &parent)
{
    ImGui::PushID(this);
    if (!showTree)
    {
        for (const auto &item: items) { item->render(window, *this); }
        ImGui::PopID();
        return;
    }

    if (openRequested)
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        openRequested = false;
    }
    const bool isOpen = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

    const bool hoveredInRowGap = ImGui::IsItemHovered();
    if (hoveredInRowGap && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && window.mSelectedItem != this)
    {
        window.mSelectedEntityChanged = true;
        window.mSelectedItem = this;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        const SceneItemPayload payload{.item = this, .parent = &parent};
        ImGui::SetDragDropPayload("SCENE_ENTITY_ITEM", &payload, sizeof(payload));
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY_ITEM",
                                                                       ImGuiDragDropFlags_AcceptNoDrawDefaultRect |
                                                                       ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
            if (payload->DataSize == sizeof(SceneItemPayload))
            {
                const auto &data = *static_cast<const SceneItemPayload *>(payload->Data);
                if (data.item != this)
                {
                    const ImVec2 rowMax = ImGui::GetItemRectMax();
                    const float lineStartX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
                    const float lineEndX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(lineStartX, rowMax.y), ImVec2(lineEndX, rowMax.y),
                                                        ImGui::GetColorU32(ImGuiCol_DragDropTarget), 3.0f);
                    if (payload->IsDelivery()) { window.queueItemMove(data.item, data.parent, this, nullptr, false); }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
    const ImVec2 rowMin = ImGui::GetItemRectMin();
    const ImVec2 rowMax = ImGui::GetItemRectMax();
    const ImVec2 rowCursor = ImGui::GetCursorScreenPos();
    const ImGuiStyle &style = ImGui::GetStyle();
    const char *icon = visible ? ICON_MS_VISIBILITY : ICON_MS_VISIBILITY_OFF;
    const float visibilityWidth = ImGui::CalcTextSize(icon).x + style.FramePadding.x * 2.0f;
    const float removeWidth = ImGui::CalcTextSize("X").x + style.FramePadding.x * 2.0f;
    const float buttonsWidth = visibilityWidth + removeWidth + style.ItemSpacing.x;
    const float buttonsStartX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - buttonsWidth;
    ImGui::SetCursorScreenPos(ImVec2(buttonsStartX, rowMin.y));
    if (ImGui::SmallButton(icon))
    {
        visible = !visible;
        const auto setVisibility = [](const auto &self, const Group &group, const bool visible) -> void
        {
            for (const auto &item: group.items)
            {
                if (const auto entityItem = dynamic_cast<EntityItem *>(item.get()))
                {
                    if (const auto entity = entityItem->entity.lock()) { entity->setVisible(visible); }
                }
                else if (const auto childGroup = dynamic_cast<Group *>(item.get()))
                {
                    childGroup->visible = visible;
                    self(self, *childGroup, visible);
                }
            }
        };
        setVisibility(setVisibility, *this, visible);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("X")) { window.queueGroupRemoval(this, &parent); }
    ImGui::SetCursorScreenPos(ImVec2(rowCursor.x, rowMax.y));
    if (isOpen)
    {
        for (const auto &item: items) { item->render(window, *this); }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Group::removeEntity(SceneEditorWindow &window, const std::weak_ptr<Entity> &entity)
{
    const auto targetEntity = entity.lock();
    if (targetEntity == nullptr) { return; }

    for (auto it = items.begin(); it != items.end();)
    {
        if (const auto entityItem = dynamic_cast<EntityItem *>(it->get());
            entityItem && entityItem->entity.lock() == targetEntity)
        {
            window.onItemRemoved(it->get());
            it = items.erase(it);
        }
        else
        {
            if (const auto group = dynamic_cast<Group *>(it->get()); group)
            {
                group->removeEntity(window, entity);
            }
            ++it;
        }
    }
}

std::unique_ptr<Item> Group::clone()
{
    auto clone = std::make_unique<Group>();
    for (const auto &item: items) { clone->items.push_back(item->clone()); }
    clone->visible = visible;
    clone->showTree = showTree;
    clone->openRequested = openRequested;
    clone->name = name;
    return std::move(clone);
}

SceneEditorWindow::SceneEditorWindow(GuiRenderer &guiRenderer)
    : GuiWindow(guiRenderer, "Scene", ImVec2(0, ImGui::GetMainViewport()->WorkPos.y), ImVec2(0, 0),
                ImGuiWindowFlags_None)
{
    const auto file = guiRenderer.getMenu("File");
    if (file != nullptr)
    {
        file->addMenuItem("New", [this] { handleNewScene(); }, "CTRL+N");
        file->addMenuItem("Open", [this] { handleOpenScene(); }, "CTRL+O");
        file->addMenuItem("Save", [this] { handleSaveScene(); }, "CTRL+S");
    }
    mEngine.getECS().addEntityAddedCallback([this](const std::shared_ptr<Entity> &entity)
    {
        if (mEngine.getSceneLoader().isLoading() || pasting) { return; }
        mAddedEntities.push_back(entity);
    });
    mEngine.getECS().addEntityRemovedCallback([this](const std::shared_ptr<Entity> &entity)
    {
        mRemovedEntities.push_back(entity);
    });
    mParentGroup = std::make_unique<Group>(false);
}

void SceneEditorWindow::renderWindowContent()
{
    for (auto entity: mAddedEntities) { mParentGroup->items.push_back(std::make_unique<EntityItem>(entity)); }
    mAddedEntities.clear();
    for (auto entity: mRemovedEntities)
    {
        mParentGroup->removeEntity(*this, entity);
    }
    mRemovedEntities.clear();

    mParentGroup->render(*this, *mParentGroup);
    const ImVec2 remaining = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("SceneRootDropTarget", ImVec2(remaining.x, std::max(remaining.y, ImGui::GetFrameHeight())));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY_ITEM",
                                                                       ImGuiDragDropFlags_AcceptNoDrawDefaultRect |
                                                                       ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
            if (payload->DataSize == sizeof(SceneItemPayload))
            {
                const auto &data = *static_cast<const SceneItemPayload *>(payload->Data);
                if (payload->IsDelivery())
                {
                    mParentGroup->openRequested = false;
                    queueItemMove(data.item, data.parent, mParentGroup.get(), nullptr, false);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
    applyQueuedItemMove();
    applyQueuedGroupRemoval();

    if (ImGui::BeginPopupContextWindow("EntitiesContext", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Entity"))
        {
            static int entityCount = 0;
            const std::string entityName =
                    entityCount == 0 ? "New Entity" : "New Entity " + std::to_string(entityCount);
            ++entityCount;
            const std::shared_ptr<Entity> &newEntity = mEngine.getECS().createEntity(entityName);
            newEntity->addComponent<TransformComponent>();
        }
        if (ImGui::MenuItem("Group")) { mParentGroup->items.push_back(std::make_unique<Group>(mParentGroup.get())); }
        ImGui::EndPopup();
    }

    if (mSelectedEntityChanged) { handleSelectedEntityChanged(); }

    const auto &input = mEngine.getInput();

    if (input.isPressedThisFrame(Input::Action::Copy)) { mSelectedItemCopy = mSelectedItem; }
    if (input.isPressedThisFrame(Input::Action::Paste))
    {
        if (mSelectedItemCopy == nullptr) { return; }
        pasting = true;
        getParent(mParentGroup.get(), mSelectedItemCopy)->items.push_back(std::move(mSelectedItemCopy->clone()));
        pasting = false;
    }
}

void SceneEditorWindow::onFramebufferSizeChanged(const int width, const int height)
{
    setPositionY(ImGui::GetMainViewport()->WorkPos.y);
    setSizeX(width / 5);
    setSizeY(ImGui::GetMainViewport()->WorkSize.y);
}

Group *SceneEditorWindow::getParent(Group *source, Item *item)
{
    if (source == nullptr || item == nullptr) { return nullptr; }
    if (source == item) { return source; }

    for (const auto &child : source->items)
    {
        if (child.get() == item)
        {
            return source;
        }
        if (const auto group = dynamic_cast<Group *>(child.get()))
        {
            if (Group *found = getParent(group, item))
            {
                return found;
            }
        }
    }
    return nullptr;
}

void SceneEditorWindow::onItemRemoved(Item *item)
{
    if (item == nullptr) { return; }

    const auto invalidateIfMatching = [this](Item *target)
    {
        if (mSelectedItem == target) { mSelectedItem = nullptr; }
        if (mSelectedItem2 == target) { mSelectedItem2 = nullptr; }
        if (mSelectedItemCopy == target) { mSelectedItemCopy = nullptr; }
    };

    if (const auto group = dynamic_cast<Group *>(item))
    {
        const auto invalidateRecursive = [&invalidateIfMatching](const auto &self, const Group &grp) -> void
        {
            for (const auto &child : grp.items)
            {
                invalidateIfMatching(child.get());
                if (const auto childGrp = dynamic_cast<const Group *>(child.get()))
                {
                    self(self, *childGrp);
                }
            }
        };
        invalidateRecursive(invalidateRecursive, *group);
    }

    invalidateIfMatching(item);
}

void SceneEditorWindow::queueItemMove(Item *item, Group *source, Group *destination, Item *target,
                                      const bool insertAfter)
{
    mMovingItem = item;
    mSourceGroup = source;
    mDestinationGroup = destination;
    mMoveTarget = target;
    mInsertAfterTarget = insertAfter;
}

void SceneEditorWindow::queueGroupRemoval(Group *group, Group *parent)
{
    mGroupToRemove = group;
    mGroupRemovalParent = parent;
}

void SceneEditorWindow::applyQueuedGroupRemoval()
{
    if (mGroupToRemove == nullptr || mGroupRemovalParent == nullptr) { return; }
    Group *group = mGroupToRemove;
    Group *parent = mGroupRemovalParent;
    mGroupToRemove = nullptr;
    mGroupRemovalParent = nullptr;

    std::vector<std::shared_ptr<Entity> > entities;
    const auto collectEntities = [&entities](const auto &self, const Group &current) -> void
    {
        for (const auto &item: current.items)
        {
            if (const auto entityItem = dynamic_cast<const EntityItem *>(item.get()))
            {
                if (const auto entity = entityItem->entity.lock()) { entities.push_back(entity); }
            }
            else if (const auto childGroup = dynamic_cast<const Group *>(item.get())) { self(self, *childGroup); }
        }
    };
    collectEntities(collectEntities, *group);
    for (const auto &entity: entities) { entity->remove(); }

    const auto groupIt = std::ranges::find_if(parent->items, [group](const auto &item) { return item.get() == group; });
    if (groupIt != parent->items.end())
    {
        onItemRemoved(group);
        parent->items.erase(groupIt);
    }
}

void SceneEditorWindow::applyQueuedItemMove()
{
    if (mMovingItem == nullptr || mSourceGroup == nullptr || mDestinationGroup == nullptr) { return; }

    const auto containsGroup = [](const auto &self, const Group *root, const Group *candidate) -> bool
    {
        if (root == candidate) { return true; }
        for (const auto &item: root->items)
        {
            if (const auto *group = dynamic_cast<const Group *>(item.get()); group && self(self, group, candidate))
            {
                return true;
            }
        }
        return false;
    };
    if (const auto *movingGroup = dynamic_cast<const Group *>(mMovingItem);
        movingGroup && containsGroup(containsGroup, movingGroup, mDestinationGroup))
    {
        mMovingItem = nullptr;
        return;
    }

    auto &sourceItems = mSourceGroup->items;
    const auto source = std::ranges::find_if(sourceItems,
                                             [this](const auto &item) { return item.get() == mMovingItem; });
    if (source == sourceItems.end())
    {
        mMovingItem = nullptr;
        return;
    }

    auto movingItem = std::move(*source);
    sourceItems.erase(source);
    auto &destinationItems = mDestinationGroup->items;
    auto insertion = destinationItems.end();
    if (mMoveTarget != nullptr)
    {
        insertion = std::ranges::find_if(destinationItems, [this](const auto &item)
        {
            return item.get() == mMoveTarget;
        });
        if (insertion != destinationItems.end() && mInsertAfterTarget) { ++insertion; }
    }
    destinationItems.insert(insertion, std::move(movingItem));
    if (mDestinationGroup->showTree) { mDestinationGroup->openRequested = true; }
    mMovingItem = nullptr;
}

void SceneEditorWindow::handleSelectedEntityChanged()
{
    mSelectedEntityChanged = false;
    if (const auto entityItem = dynamic_cast<EntityItem *>(mSelectedItem2))
    {
        const auto copy = entityItem->entity.lock();
        if (copy != nullptr && copy->hasComponent<ModelComponent>())
        {
            copy->getComponent<ModelComponent>().outline = false;
        }
    }

    mSelectedItem2 = mSelectedItem;

    if (const auto entityItem = dynamic_cast<EntityItem *>(mSelectedItem2))
    {
        const auto selected = entityItem->entity.lock();
        if (selected == nullptr) { return; }
        mGuiRenderer.getWindow<PropertyWindow>().open();
        for (Component *component: selected->getComponents()) { component->codec.markDirty(); }

        if (selected->hasComponent<ModelComponent>())
        {
            auto &modelComponent = selected->getComponent<ModelComponent>();
            modelComponent.outline = true;
        }
    }
}

void SceneEditorWindow::handleNewScene() const
{
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("New Scene");
    window.setDirectoryPath("assets/scenes");
    window.setTypeFilters({".json"});
    window.setFlags(FileExplorerFlags_EnterNewFilename);
    window.setSelectedPathsCallback([this](const std::vector<std::filesystem::path> &path)
    {
        mEngine.getSceneLoader().setScenePath(path[0]);
        mEngine.getSceneLoader().clearScene();
        std::ofstream file(path[0]);
        if (file.is_open()) { file.close(); }
    });
    window.open();
}

void SceneEditorWindow::handleOpenScene() const
{
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Open Scene");
    window.setDirectoryPath("assets/scenes");
    window.setTypeFilters({".json"});
    window.setFlags(FileExplorerFlags_AllowSelectionOnDoubleClick);
    window.setSelectedPathsCallback([this](const std::vector<std::filesystem::path> &path)
    {
        auto &loadingWindow = mGuiRenderer.getWindow<LoadingWindow>();
        loadingWindow.setPositionX(
            ImGui::GetIO().DisplaySize.x / 2.0f - loadingWindow.getSize().x / 2.0f);
        loadingWindow.setPositionY(
            ImGui::GetIO().DisplaySize.y / 2.0f - loadingWindow.getSize().y / 2.0f);
        loadingWindow.open();
        mEngine.getSceneLoader().clearScene();
        mEngine.getSceneLoader().setScenePath(path[0]);
        mEngine.getSceneLoader().loadScene(path[0]).thenAccept([&] { loadingWindow.close(); },
                                                               mEngine.getGlExecutor());
    });
    window.open();
}

void SceneEditorWindow::handleSaveScene() const
{
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Save Scene");
    window.setDirectoryPath("assets/scenes");
    window.setTypeFilters({".json"});
    window.setFlags(FileExplorerFlags_EnterNewFilename);
    window.setSelectedPaths({mEngine.getSceneLoader().getScenePath()});
    window.setSelectedPathsCallback([this](const std::vector<std::filesystem::path> &path)
    {
        mEngine.getSceneLoader().exportScene(path[0]);
    });
    window.open();
}
