#include "AnimatorWindow.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "FileExplorerWindow.h"
#include "core/Engine.h"
#include "ecs/component/Animator.h"
#include "ecs/core/ECS.h"

namespace {

constexpr Ease allEases[] = {
    Ease::None, Ease::Linear,
    Ease::InSine, Ease::OutSine, Ease::InOutSine,
    Ease::InQuad, Ease::OutQuad, Ease::InOutQuad,
    Ease::InCubic, Ease::OutCubic, Ease::InOutCubic,
    Ease::InQuart, Ease::OutQuart, Ease::InOutQuart,
    Ease::InQuint, Ease::OutQuint, Ease::InOutQuint,
    Ease::InExpo, Ease::OutExpo, Ease::InOutExpo,
    Ease::InCirc, Ease::OutCirc, Ease::InOutCirc,
    Ease::InElastic, Ease::OutElastic, Ease::InOutElastic
};

AnimValue getCurrentCodecValue(const Entity &entity, const std::string &propName)
{
    for (Component *comp : entity.getComponents())
    {
        if (comp == nullptr) { continue; }
        if (comp->typeId == getComponentTypeId<Animator>()) { continue; }

        auto it = comp->codec.properties.find(propName);
        if (it != comp->codec.properties.end())
        {
            auto [type, prop] = it->second;
            return std::visit([](auto *ptr) -> AnimValue {
                if (ptr == nullptr) { return 0.0f; }
                using T = std::decay_t<decltype(*ptr)>;
                if constexpr (std::is_same_v<T, bool>) { return *ptr; }
                else if constexpr (std::is_same_v<T, int>) { return *ptr; }
                else if constexpr (std::is_same_v<T, float>) { return *ptr; }
                else if constexpr (std::is_same_v<T, glm::vec2>) { return *ptr; }
                else if constexpr (std::is_same_v<T, glm::vec3>) { return *ptr; }
                else if constexpr (std::is_same_v<T, glm::vec4>) { return *ptr; }
                else if constexpr (std::is_same_v<T, glm::quat>) { return *ptr; }
                else { return 0.0f; }
            }, prop);
        }

        // Check sub-codecs
        for (const auto &[name, pair] : comp->codec.properties)
        {
            if (pair.first == Codec::Type::CODEC)
            {
                if (auto *subCodec = std::get<Codec *>(pair.second))
                {
                    auto subIt = subCodec->properties.find(propName);
                    if (subIt != subCodec->properties.end())
                    {
                        auto [type, prop] = subIt->second;
                        return std::visit([](auto *ptr) -> AnimValue {
                            if (!ptr) return 0.0f;
                            using T = std::decay_t<decltype(*ptr)>;
                            if constexpr (std::is_same_v<T, bool>) return *ptr;
                            else if constexpr (std::is_same_v<T, int>) return *ptr;
                            else if constexpr (std::is_same_v<T, float>) return *ptr;
                            else if constexpr (std::is_same_v<T, glm::vec2>) return *ptr;
                            else if constexpr (std::is_same_v<T, glm::vec3>) return *ptr;
                            else if constexpr (std::is_same_v<T, glm::vec4>) return *ptr;
                            else if constexpr (std::is_same_v<T, glm::quat>) return *ptr;
                            else return 0.0f;
                        }, prop);
                    }
                }
            }
        }
    }
    return 0.0f;
}

} // namespace

AnimatorWindow::AnimatorWindow(GuiRenderer &editor)
    : GuiWindow(editor, "Animation Editor", ImVec2(100, 100), ImVec2(800, 450))
{
}

void AnimatorWindow::renderWindowContent()
{
    const ECS &ecs = mEngine.getECS();
    const auto entities = ecs.getEntities();

    // Auto-select first entity with Animator or any entity if none selected
    Entity *selectedEntity = nullptr;
    if (mSelectedEntityId.has_value())
    {
        for (auto &e : entities)
        {
            if (e && e->getId() == *mSelectedEntityId && !e->isRemoved())
            {
                selectedEntity = e.get();
                break;
            }
        }
    }

    if (!selectedEntity)
    {
        for (auto &e : entities)
        {
            if (e && !e->isRemoved() && e->hasComponent<Animator>())
            {
                selectedEntity = e.get();
                mSelectedEntityId = e->getId();
                break;
            }
        }
        if (!selectedEntity && !entities.empty())
        {
            for (auto &e : entities)
            {
                if (e && !e->isRemoved())
                {
                    selectedEntity = e.get();
                    mSelectedEntityId = e->getId();
                    break;
                }
            }
        }
    }

    // Top Entity Selector Bar
    ImGui::Text("Entite Cible:");
    ImGui::SameLine();
    const std::string currentEntityName = selectedEntity ? selectedEntity->getName() + " (ID: " + std::to_string(selectedEntity->getId()) + ")" : "Aucune entite";
    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::BeginCombo("##EntitySelect", currentEntityName.c_str()))
    {
        for (auto &e : entities)
        {
            if (!e || e->isRemoved()) continue;
            const bool isSelected = (selectedEntity && selectedEntity->getId() == e->getId());
            std::string label = e->getName() + " (ID: " + std::to_string(e->getId()) + ")" + (e->hasComponent<Animator>() ? " [Animator]" : "");
            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                mSelectedEntityId = e->getId();
                selectedEntity = e.get();
                mSelectedTrackIndex = -1;
                mSelectedKeyframeIndex = -1;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (!selectedEntity)
    {
        ImGui::TextDisabled("Please selected an entity.");
        return;
    }

    // Ensure entity has Animator component
    if (!selectedEntity->hasComponent<Animator>())
    {
        ImGui::SameLine();
        if (ImGui::Button("Add an Animator component to this entity"))
        {
            selectedEntity->addComponent<Animator>();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("This entity does not have an Animator component.");
        return;
    }

    auto &animator = selectedEntity->getComponent<Animator>();

    drawHeader(animator, *selectedEntity);
    ImGui::Separator();

    // Split window horizontally: Left = Tracks & Inspector, Right = Timeline
    constexpr float leftWidth = 280.0f;
    const float rightWidth = ImGui::GetContentRegionAvail().x - leftWidth - 10.0f;

    ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);
    {
        drawTrackList(animator, *selectedEntity);
        ImGui::Separator();
        drawKeyframeInspector(animator);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(rightWidth, 0), true);
    {
        drawTimeline(animator, *selectedEntity);
    }
    ImGui::EndChild();
}

void AnimatorWindow::drawHeader(Animator &animator, Entity &entity)
{
    // Controls: Play/Pause, Stop, Time, Duration, Speed, Loop, Record
    if (ImGui::Button(animator.isPlaying ? "Pause ||" : "Play >"))
    {
        animator.isPlaying = !animator.isPlaying;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop []"))
    {
        animator.stop();
    }
    ImGui::SameLine();

    ImGui::Text("Time:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::DragFloat("##CurrentTime", &animator.currentTime, 0.01f, 0.0f, animator.duration, "%.2f s"))
    {
        animator.setTime(animator.currentTime);
    }

    ImGui::SameLine();
    ImGui::Text("/ Duration:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::DragFloat("##Duration", &animator.duration, 0.1f, 0.1f, 60.0f, "%.1f s");

    ImGui::SameLine();
    ImGui::Checkbox("Loop", &animator.isLooping);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    ImGui::DragFloat("Speed", &animator.speed, 0.1f, 0.0f, 10.0f, "%.1fx");

    ImGui::SameLine();
    if (animator.isRecording)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    }
    if (ImGui::Button(animator.isRecording ? "Rec [ON]" : "Rec"))
    {
        animator.isRecording = !animator.isRecording;
    }
    if (animator.isRecording)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        handleExportAnimation(animator);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Export the animation as a JSON file");
    }

    ImGui::SameLine();
    if (ImGui::Button("Import"))
    {
        handleImportAnimation(animator);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Import an animation from a JSON file");
    }
}

void AnimatorWindow::drawTimeline(Animator &animator, Entity &entity)
{
    constexpr float TICK_INTERVAL = 0.1f;
    constexpr float TRACK_HEIGHT = 32.0f;
    constexpr float HEADER_HEIGHT = 28.0f;

    ImGui::Text("Timeline");
    ImGui::SameLine();
    ImGui::SliderFloat("Zoom", &mPixelsPerSecond, 30.0f, 300.0f, "%.0f px/s");

    float totalWidth = std::max(animator.duration * mPixelsPerSecond + 100.0f, ImGui::GetContentRegionAvail().x);
    float totalHeight = HEADER_HEIGHT + animator.tracks.size() * TRACK_HEIGHT + 40.0f;

    ImGui::BeginChild("TimelineScrollRegion", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();

        // 1. Invisible button covering the entire timeline canvas to capture mouse input
        // and prevent dragging the main window when clicking/dragging on the timeline.
        ImGui::InvisibleButton("##TimelineCanvas", ImVec2(totalWidth, totalHeight), ImGuiButtonFlags_MouseButtonLeft);
        bool isCanvasActive = ImGui::IsItemActive();
        bool isCanvasHovered = ImGui::IsItemHovered();

        // 2. Draw Time Header & Grid
        drawList->AddRectFilled(origin, ImVec2(origin.x + totalWidth, origin.y + HEADER_HEIGHT), IM_COL32(35, 35, 38, 255));

        for (float t = 0.0f; t <= animator.duration + 0.01f; t += TICK_INTERVAL)
        {
            float x = origin.x + t * mPixelsPerSecond;
            bool isMajor = (std::fmod(t + 0.001f, 1.0f) < 0.05f);

            drawList->AddLine(
                ImVec2(x, origin.y + (isMajor ? 0.0f : HEADER_HEIGHT * 0.5f)),
                ImVec2(x, origin.y + totalHeight),
                isMajor ? IM_COL32(120, 120, 120, 150) : IM_COL32(60, 60, 60, 80),
                1.0f
            );

            if (isMajor)
            {
                char label[16];
                snprintf(label, sizeof(label), "%.1fs", t);
                drawList->AddText(ImVec2(x + 4.0f, origin.y + 4.0f), IM_COL32(200, 200, 200, 255), label);
            }
        }

        // 3. Check for clicking keyframes first
        bool clickedAnyKeyframe = false;
        ImVec2 mousePos = ImGui::GetMousePos();

        // 4. Draw Tracks & Keyframes
        for (size_t trackIdx = 0; trackIdx < animator.tracks.size(); ++trackIdx)
        {
            auto &track = animator.tracks[trackIdx];
            float y = origin.y + HEADER_HEIGHT + trackIdx * TRACK_HEIGHT;

            bool isTrackSelected = (mSelectedTrackIndex == static_cast<int>(trackIdx));
            ImU32 trackBg = isTrackSelected ? IM_COL32(45, 60, 80, 200) : (trackIdx % 2 == 0 ? IM_COL32(30, 30, 32, 200) : IM_COL32(25, 25, 27, 200));

            drawList->AddRectFilled(
                ImVec2(origin.x, y),
                ImVec2(origin.x + totalWidth, y + TRACK_HEIGHT),
                trackBg
            );

            // Track name overlay on track bar
            drawList->AddText(ImVec2(origin.x + 8.0f, y + 6.0f), IM_COL32(220, 220, 220, 180), track.propertyName.c_str());

            // Keyframe Diamonds
            for (size_t kfIdx = 0; kfIdx < track.keyframes.size(); ++kfIdx)
            {
                auto &kf = track.keyframes[kfIdx];
                float kfX = origin.x + kf.time * mPixelsPerSecond;
                float kfY = y + TRACK_HEIGHT * 0.5f;
                float diamondSize = 6.0f;

                bool isKfSelected = (isTrackSelected && mSelectedKeyframeIndex == static_cast<int>(kfIdx));

                ImVec2 p[4] = {
                    {kfX, kfY - diamondSize},
                    {kfX + diamondSize, kfY},
                    {kfX, kfY + diamondSize},
                    {kfX - diamondSize, kfY}
                };

                ImU32 fillColor = isKfSelected ? IM_COL32(255, 215, 0, 255) : IM_COL32(80, 180, 255, 255);
                ImU32 outlineColor = IM_COL32(255, 255, 255, 255);

                drawList->AddConvexPolyFilled(p, 4, fillColor);
                drawList->AddPolyline(p, 4, outlineColor, ImDrawFlags_Closed, 1.5f);

                // Hover / Click Keyframe
                if (mousePos.x >= kfX - diamondSize && mousePos.x <= kfX + diamondSize &&
                    mousePos.y >= kfY - diamondSize && mousePos.y <= kfY + diamondSize)
                {
                    if (ImGui::IsMouseClicked(0))
                    {
                        mSelectedTrackIndex = static_cast<int>(trackIdx);
                        mSelectedKeyframeIndex = static_cast<int>(kfIdx);
                        mIsDraggingKeyframe = true;
                        clickedAnyKeyframe = true;
                    }
                }
            }
        }

        // Dragging keyframe
        if (mIsDraggingKeyframe)
        {
            if (ImGui::IsMouseDown(0) && mSelectedTrackIndex >= 0 && mSelectedTrackIndex < static_cast<int>(animator.tracks.size()))
            {
                auto &track = animator.tracks[mSelectedTrackIndex];
                if (mSelectedKeyframeIndex >= 0 && mSelectedKeyframeIndex < static_cast<int>(track.keyframes.size()))
                {
                    float mouseTime = (ImGui::GetMousePos().x - origin.x) / mPixelsPerSecond;
                    track.keyframes[mSelectedKeyframeIndex].time = glm::clamp(mouseTime, 0.0f, animator.duration);
                    track.sortKeyframes();
                }
            }
            else
            {
                mIsDraggingKeyframe = false;
            }
        }

        // 5. Playhead (Ligne de lecture)
        float playheadX = origin.x + animator.currentTime * mPixelsPerSecond;
        drawList->AddLine(
            ImVec2(playheadX, origin.y),
            ImVec2(playheadX, origin.y + totalHeight),
            IM_COL32(255, 70, 70, 255),
            2.0f
        );

        // Playhead Top Pointer
        ImVec2 pointer[3] = {
            {playheadX - 6.0f, origin.y},
            {playheadX + 6.0f, origin.y},
            {playheadX, origin.y + 12.0f}
        };
        drawList->AddConvexPolyFilled(pointer, 3, IM_COL32(255, 70, 70, 255));

        // Click / Drag on Header or Canvas to move Playhead (Scrubber)
        if (!mIsDraggingKeyframe && !clickedAnyKeyframe && (isCanvasActive || (isCanvasHovered && ImGui::IsMouseDown(0))))
        {
            if (mousePos.x >= origin.x && mousePos.x <= origin.x + totalWidth &&
                mousePos.y >= origin.y && mousePos.y <= origin.y + totalHeight)
            {
                float newTime = (mousePos.x - origin.x) / mPixelsPerSecond;
                animator.setTime(newTime);

                // Auto-record if recording enabled
                if (animator.isRecording && mSelectedTrackIndex >= 0 && mSelectedTrackIndex < static_cast<int>(animator.tracks.size()))
                {
                    auto &track = animator.tracks[mSelectedTrackIndex];
                    AnimValue val = getCurrentCodecValue(entity, track.propertyName);
                    track.addOrUpdateKeyframe(animator.currentTime, val, Ease::Linear);
                }
            }
        }
    }
    ImGui::EndChild();
}

void AnimatorWindow::drawTrackList(Animator &animator, Entity &entity)
{
    ImGui::Text("Animation tracks (%zu)", animator.tracks.size());
    drawAddTrackMenu(animator, entity);
    ImGui::Spacing();

    for (size_t i = 0; i < animator.tracks.size(); ++i)
    {
        auto &track = animator.tracks[i];
        ImGui::PushID(static_cast<int>(i));

        const bool isSelected = (mSelectedTrackIndex == static_cast<int>(i));
        std::string label = track.propertyName.empty() ? ("Track " + std::to_string(i)) : track.propertyName;

        const float availWidth = ImGui::GetContentRegionAvail().x;
        constexpr float buttonsWidth = 52.0f; // Width for [K] and [x] buttons + spacing
        const float selectableWidth = std::max(20.0f, availWidth - buttonsWidth);

        if (ImGui::Selectable(label.c_str(), isSelected, 0, ImVec2(selectableWidth, 22.0f)))
        {
            mSelectedTrackIndex = static_cast<int>(i);
            mSelectedKeyframeIndex = -1;
        }

        // Capture keyframe button "K"
        ImGui::SameLine();
        if (ImGui::Button("K", ImVec2(22.0f, 22.0f)))
        {
            AnimValue curVal = getCurrentCodecValue(entity, track.propertyName);
            track.addOrUpdateKeyframe(animator.currentTime, curVal, Ease::None);
            mSelectedTrackIndex = static_cast<int>(i);
            // Select the newly added or updated keyframe
            for (size_t kIdx = 0; kIdx < track.keyframes.size(); ++kIdx)
            {
                if (std::abs(track.keyframes[kIdx].time - animator.currentTime) < 0.001f)
                {
                    mSelectedKeyframeIndex = static_cast<int>(kIdx);
                    break;
                }
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Add/Update a keyframe (current value at t = %.2fs)", animator.currentTime);
        }

        // Delete track button "x"
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("x", ImVec2(22.0f, 22.0f)))
        {
            animator.removeTrack(i);
            if (mSelectedTrackIndex == static_cast<int>(i))
            {
                mSelectedTrackIndex = -1;
                mSelectedKeyframeIndex = -1;
            }
            else if (mSelectedTrackIndex > static_cast<int>(i))
            {
                mSelectedTrackIndex--;
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
            break;
        }
        ImGui::PopStyleColor();

        ImGui::PopID();
    }
}

void AnimatorWindow::drawKeyframeInspector(Animator &animator)
{
    ImGui::Text("Keyframe Inspector");
    ImGui::Separator();

    if (mSelectedTrackIndex < 0 || mSelectedTrackIndex >= static_cast<int>(animator.tracks.size()))
    {
        ImGui::TextDisabled("No track selected.");
        return;
    }

    auto &track = animator.tracks[mSelectedTrackIndex];
    ImGui::Text("Track: %s", track.propertyName.c_str());

    if (mSelectedKeyframeIndex < 0 || mSelectedKeyframeIndex >= static_cast<int>(track.keyframes.size()))
    {
        ImGui::TextDisabled("No keyframe selected.");
        return;
    }

    auto &kf = track.keyframes[mSelectedKeyframeIndex];

    // Time
    float timeVal = kf.time;
    if (ImGui::DragFloat("Time (s)", &timeVal, 0.01f, 0.0f, animator.duration, "%.3f"))
    {
        kf.time = glm::clamp(timeVal, 0.0f, animator.duration);
        track.sortKeyframes();
    }

    // Ease
    if (ImGui::BeginCombo("Easing", easeToString(kf.ease)))
    {
        for (const Ease e : allEases)
        { const bool isSelected = (kf.ease == e);
            if (ImGui::Selectable(easeToString(e), isSelected))
            {
                kf.ease = e;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Value Editor based on variant type
    std::visit([]<typename T0>(T0 &val) {
        using T = std::decay_t<T0>;
        if constexpr (std::is_same_v<T, bool>)
        {
            ImGui::Checkbox("Value (bool)", &val);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            ImGui::DragInt("Value (int)", &val, 1);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            ImGui::DragFloat("Value (float)", &val, 0.05f);
        }
        else if constexpr (std::is_same_v<T, glm::vec2>)
        {
            ImGui::DragFloat2("Value (vec2)", &val.x, 0.05f);
        }
        else if constexpr (std::is_same_v<T, glm::vec3>)
        {
            ImGui::DragFloat3("Value (vec3)", &val.x, 0.05f);
        }
        else if constexpr (std::is_same_v<T, glm::vec4>)
        {
            ImGui::DragFloat4("Value (vec4)", &val.x, 0.05f);
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            glm::vec3 euler = glm::degrees(glm::eulerAngles(val));
            if (ImGui::DragFloat3("Euler rotation", &euler.x, 0.5f))
            {
                val = glm::quat(glm::radians(euler));
            }
        }
    }, kf.value);

    if (ImGui::Button("Remove Keyframe", ImVec2(-1, 24)))
    {
        track.removeKeyframe(mSelectedKeyframeIndex);
        mSelectedKeyframeIndex = -1;
    }
}

void AnimatorWindow::drawAddTrackMenu(Animator &animator, Entity &entity)
{
    if (ImGui::Button("+ Add Track", ImVec2(-1, 24)))
    {
        ImGui::OpenPopup("AddTrackPopup");
    }

    if (ImGui::BeginPopup("AddTrackPopup"))
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Properties:");
        ImGui::Separator();

        for (Component *comp : entity.getComponents())
        {
            if (!comp) continue;

            for (const auto &[propName, pair] : comp->codec.properties)
            {
                if (pair.first == Codec::Type::CODEC)
                {
                    if (auto *subCodec = std::get<Codec *>(pair.second))
                    {
                        for (const auto &[subPropName, subPair] : subCodec->properties)
                        {
                            if (subPair.first == Codec::Type::OPTION || subPair.first == Codec::Type::CODEC) continue;

                            bool alreadyExists = (animator.findTrack("", subPropName) != nullptr);
                            if (alreadyExists) ImGui::BeginDisabled();
                            if (ImGui::MenuItem(subPropName.c_str()))
                            {
                                auto &t = animator.getOrCreateTrack("", subPropName);
                                AnimValue val = getCurrentCodecValue(entity, subPropName);
                                t.addOrUpdateKeyframe(animator.currentTime, val, Ease::Linear);
                            }
                            if (alreadyExists) ImGui::EndDisabled();
                        }
                    }
                    continue;
                }

                if (pair.first == Codec::Type::OPTION) continue;

                bool alreadyExists = (animator.findTrack("", propName) != nullptr);
                if (alreadyExists) ImGui::BeginDisabled();
                if (ImGui::MenuItem(propName.c_str()))
                {
                    auto &t = animator.getOrCreateTrack("", propName);
                    AnimValue val = getCurrentCodecValue(entity, propName);
                    t.addOrUpdateKeyframe(animator.currentTime, val, Ease::Linear);
                }
                if (alreadyExists) ImGui::EndDisabled();
            }
        }
        ImGui::EndPopup();
    }
}

void AnimatorWindow::handleExportAnimation(Animator &animator) const
{
    if (!mGuiRenderer.isRegisteredWindow<FileExplorerWindow>()) return;
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Exporter l'animation");
    window.setDirectoryPath("assets/scenes");
    window.setTypeFilters({".json"});
    window.setFlags(FileExplorerFlags_EnterNewFilename);
    window.setSelectedPathsCallback([&animator](const std::vector<std::filesystem::path> &paths) {
        if (!paths.empty())
        {
            std::filesystem::path exportPath = paths[0];
            if (exportPath.extension() != ".json")
            {
                exportPath += ".json";
            }
            std::ofstream file(exportPath);
            if (file.is_open())
            {
                file << animator.toJson().dump(4);
                file.close();
            }
        }
    });
    window.open();
}

void AnimatorWindow::handleImportAnimation(Animator &animator)
{
    if (!mGuiRenderer.isRegisteredWindow<FileExplorerWindow>()) return;
    auto &window = mGuiRenderer.getWindow<FileExplorerWindow>();
    window.setTitle("Importer l'animation");
    window.setDirectoryPath("assets/scenes");
    window.setTypeFilters({".json"});
    window.setFlags(FileExplorerFlags_AllowSelectionOnDoubleClick);
    window.setSelectedPathsCallback([this, &animator](const std::vector<std::filesystem::path> &paths) {
        if (!paths.empty())
        {
            std::ifstream file(paths[0]);
            if (file.is_open())
            {
                try
                {
                    json j;
                    file >> j;
                    animator.fromJson(j);
                    mSelectedTrackIndex = -1;
                    mSelectedKeyframeIndex = -1;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Erreur lors de l'import de l'animation: " << e.what() << std::endl;
                }
                file.close();
            }
        }
    });
    window.open();
}
