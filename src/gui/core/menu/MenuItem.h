#ifndef OPENGL_GUI_MENU_ITEM_H
#define OPENGL_GUI_MENU_ITEM_H

#include <functional>
#include <string>

#include "IMenu.h"

class MenuItem : public IMenu
{
public:
    MenuItem(const std::string &label, const std::string &string, const std::string &shortcut,
             const std::function<void()> &function, bool *selected);

    void render() const override;

private:
    std::string mLabel;
    std::string mTooltip;
    std::string mShortcut;
    std::function<void()> mOnClick;
    bool *mSelected;
};

#endif //OPENGL_GUI_MENU_ITEM_H
