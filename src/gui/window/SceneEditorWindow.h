#ifndef OPENGL_SCENE_EDITOR_WINDOW_H
#define OPENGL_SCENE_EDITOR_WINDOW_H

#include <mutex>
#include <vector>

#include "ecs/core/Entity.h"
#include "gui/core/GuiWindow.h"

class Entity;
class SceneEditorWindow;
struct Group;

struct Item
{
    virtual ~Item() = default;

    virtual void render(SceneEditorWindow &window, Group &parent) = 0;

    virtual std::string getName() = 0;

    virtual void setName(const std::string &name) = 0;

    virtual std::unique_ptr<Item> clone() = 0;
};

struct EntityItem : Item
{
    std::weak_ptr<Entity> entity;

    explicit EntityItem(const std::shared_ptr<Entity> &entity) : entity(entity) {}

    void render(SceneEditorWindow &window, Group &parent) override;

    std::string getName() override;

    void setName(const std::string &name) override;

    std::unique_ptr<Item> clone() override { return std::make_unique<EntityItem>(entity.lock()->clone()); }
};

struct Group : Item
{
    std::vector<std::unique_ptr<Item> > items;
    bool visible = true;
    bool showTree = true;
    bool openRequested = false;
    std::string name = "Group";

    explicit Group(const bool showTree = true) : showTree(showTree) {}

    void render(SceneEditorWindow &window, Group &parent) override;

    void removeEntity(SceneEditorWindow &window, const std::weak_ptr<Entity> &entity);

    std::string getName() override { return name; }

    void setName(const std::string &n) override { this->name = n; }

    std::unique_ptr<Item> clone() override;
};

class SceneEditorWindow : public GuiWindow
{
public:
    explicit SceneEditorWindow(GuiRenderer &guiRenderer);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override;

    Item *getSelectedItem() const { return mSelectedItem; }

    Group *getParentGroup() const { return mParentGroup.get(); }

    void clear()
    {
        mParentGroup->items.clear();
        mSelectedItem = nullptr;
        mSelectedItem2 = nullptr;
        mSelectedItemCopy = nullptr;
        mSelectedEntityChanged = false;
        mMovingItem = nullptr;
        mSourceGroup = nullptr;
        mDestinationGroup = nullptr;
        mMoveTarget = nullptr;
    }

private:
    friend struct EntityItem;

    friend struct Group;

    void onItemRemoved(Item *item);

    static Group *getParent(Group *source, Item *item);

    void queueItemMove(Item *item, Group *source, Group *destination, Item *target, bool insertAfter);

    void queueGroupRemoval(Group *group, Group *parent);

    void applyQueuedGroupRemoval();

    void applyQueuedItemMove();

    void handleSelectedEntityChanged();

    void handleNewScene() const;

    void handleOpenScene() const;

    void handleSaveScene() const;

    std::mutex mMutex;
    std::unique_ptr<Group> mParentGroup;
    std::vector<std::shared_ptr<Entity> > mAddedEntities;
    std::vector<std::shared_ptr<Entity> > mRemovedEntities;
    Item *mSelectedItem = nullptr;
    Item *mSelectedItem2 = nullptr;
    Item *mSelectedItemCopy = nullptr;
    bool mSelectedEntityChanged = false;
    bool pasting = false;
    Item *mMovingItem = nullptr;
    Group *mSourceGroup = nullptr;
    Group *mDestinationGroup = nullptr;
    Item *mMoveTarget = nullptr;
    bool mInsertAfterTarget = false;
    Group *mGroupToOpen = nullptr;
    Group *mGroupToRemove = nullptr;
    Group *mGroupRemovalParent = nullptr;
};


#endif //OPENGL_SCENE_EDITOR_WINDOW_H
