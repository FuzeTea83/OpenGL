#include "MenuItem.h"

#include <imgui.h>
#include <stdexcept>

MenuItem::MenuItem(const std::string &label, const std::string &string, const std::string &shortcut,
                   const std::function<void()> &function, bool *selected)
    : mLabel(label), mTooltip(string), mShortcut(shortcut), mOnClick(function), mSelected(selected)
{
    if (mLabel.empty()) { throw std::invalid_argument("Label cannot be empty"); }
}

void MenuItem::render() const
{
    if (ImGui::MenuItem(mLabel.c_str(), mShortcut.empty() ? nullptr : mShortcut.c_str(), mSelected))
    {
        if (mOnClick) { mOnClick(); }
    }

    if (!mTooltip.empty() && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", mTooltip.c_str()); }
}
