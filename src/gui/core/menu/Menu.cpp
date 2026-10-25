#include "Menu.h"

#include <imgui.h>
#include <stdexcept>

Menu::Menu(const std::string &label) : mLabel(label)
{
    if (mLabel.empty()) { throw std::invalid_argument("Label cannot be empty"); }
}

void Menu::render() const
{
    if (ImGui::BeginMenu(mLabel.c_str()))
    {
        for (const auto &subMenu: mSubMenus) { subMenu->render(); }
        ImGui::EndMenu();
    }
}

Menu &Menu::setLabel(const std::string &label)
{
    mLabel = label;
    return *this;
}

Menu &Menu::addSubMenu(std::unique_ptr<Menu> subMenu)
{
    mSubMenus.push_back(std::move(subMenu));
    return *this;
}

Menu &Menu::addMenuItem(const std::string &label, const std::function<void()> &onClick,
                                          const std::string &shortcut, const std::string &tooltip, bool *selected)
{
    mSubMenus.push_back(std::make_unique<MenuItem>(label, tooltip, shortcut, onClick, selected));
    return *this;
}

Menu &Menu::addMenuItem(const MenuItem &menuItem)
{
    mSubMenus.push_back(std::make_unique<MenuItem>(menuItem));
    return *this;
}

Menu::Menu(const std::string &label, std::vector<std::unique_ptr<IMenu>> &&subButtons)
    : mLabel(label), mSubMenus(std::move(subButtons)) {}
