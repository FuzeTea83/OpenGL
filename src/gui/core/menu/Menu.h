#ifndef OPENGL_GUI_MENU_H
#define OPENGL_GUI_MENU_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "IMenu.h"
#include "MenuItem.h"

class Menu : public IMenu
{
public:
    explicit Menu(const std::string &label);

    ~Menu() override = default;

    Menu(const Menu &) = delete;

    Menu &operator=(const Menu &) = delete;

    Menu(Menu &&) noexcept = default;

    Menu &operator=(Menu &&) noexcept = default;

    void render() const override;

    Menu &setLabel(const std::string &label);

    Menu &addSubMenu(std::unique_ptr<Menu> subMenu);

    Menu &addMenuItem(const std::string &label, const std::function<void()> &onClick,
                      const std::string &shortcut = "", const std::string &tooltip = "", bool *selected = nullptr);

    Menu &addMenuItem(const MenuItem &menuItem);

    std::string &getLabel() { return mLabel; }

    std::vector<std::unique_ptr<IMenu> > &getSubMenus() { return mSubMenus; }


private:
    Menu(const std::string &label, std::vector<std::unique_ptr<IMenu> > &&subButtons);

    std::string mLabel;
    std::vector<std::unique_ptr<IMenu> > mSubMenus;
};

#endif //OPENGL_GUI_MENU_H
