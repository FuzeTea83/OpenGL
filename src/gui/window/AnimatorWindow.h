#ifndef OPENGL_ANIMATION_WINDOW_H
#define OPENGL_ANIMATION_WINDOW_H

#include <vector>
#include <string>
#include <optional>

#include "gui/core/GuiWindow.h"
#include "ecs/component/Animator.h"
#include "ecs/core/Entity.h"

class AnimatorWindow : public GuiWindow
{
public:
    explicit AnimatorWindow(GuiRenderer &editor);

    void renderWindowContent() override;

    void onFramebufferSizeChanged(int width, int height) override {}

private:
    void drawHeader(Animator &animator, Entity &entity);
    void drawTimeline(Animator &animator, Entity &entity);
    void drawTrackList(Animator &animator, Entity &entity);
    void drawKeyframeInspector(Animator &animator);

    static void drawAddTrackMenu(Animator &animator, Entity &entity);
    void handleExportAnimation(Animator &animator) const;
    void handleImportAnimation(Animator &animator);

    std::optional<EntityId> mSelectedEntityId;
    int mSelectedTrackIndex = -1;
    int mSelectedKeyframeIndex = -1;
    bool mIsDraggingKeyframe = false;
    float mTimelineScrollX = 0.0f;
    float mPixelsPerSecond = 100.0f;
};

#endif //OPENGL_ANIMATION_WINDOW_H
