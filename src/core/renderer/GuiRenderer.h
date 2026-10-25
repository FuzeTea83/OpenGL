#ifndef OPENGL_GUI_RENDERER_H
#define OPENGL_GUI_RENDERER_H

#include "gui/core/GuiWindow.h"
#include "gui/core/menu/Menu.h"
#include "utils/Codec.h"

class Engine;

class GuiRenderer
{
public:
    explicit GuiRenderer(const Engine &engine);

    ~GuiRenderer();

    void render();

    void toggleEditor() { mShowEditor = !mShowEditor; }

    bool isEditorVisible() const { return mShowEditor; }

    GuiWindow &getWindow(const WindowId id) const
    {
        if (!isRegisteredWindow(id))
        {
            throw std::runtime_error("Window with id " + std::to_string(id) + " is not registered.");
        }
        return *mWindows.at(id);
    }

    template<typename T>
    T &getWindow() const { return static_cast<T &>(getWindow(getWindowTypeId<T>())); }

    const Engine &getEngine() const;

    static void renderCodec(const Codec &codec);

    void enableAllWindows() const;

    template<typename T>
    T &registerWindow(const bool addViewMenu = true)
    {
        static_assert(std::is_base_of_v<GuiWindow, T>, "T must be derived from GuiWindow");
        auto window = std::make_unique<T>(*this);
        auto &windowRef = *window;
        mWindows[getWindowTypeId<T>()] = std::move(window);

        if (!addViewMenu) return windowRef;
        auto &castedWindow = static_cast<GuiWindow &>(windowRef);
        mViewMenu->addMenuItem(
            castedWindow.getTitle(),
            [this, &castedWindow]
            {
                if (castedWindow.mOpen) { castedWindow.open(); }
                else { castedWindow.close(); }
            },
            "", "", &castedWindow.mOpen);
        return windowRef;
    }

    bool isRegisteredWindow(const WindowId id) const { return mWindows.contains(id); }

    template<typename T>
    bool isRegisteredWindow() const { return isRegisteredWindow(getWindowTypeId<T>()); }

    void disableAllWindowsExcept(const GuiWindow &window) const;

    Menu* createMenu(std::string label, const unsigned char position)
    {
        auto menu = std::make_unique<Menu>(label);
        mMenus[position] = std::move(menu);
        return mMenus[position].get();
    }

    Menu* getMenu(const std::string &label)
    {
        for (const auto &menu: mMenus | std::views::values)
        {
            if (menu->getLabel() == label) { return menu.get(); }
        }
        return nullptr;
    }

private:
    const Engine &mEngine;
    bool mShowEditor = false;
    std::map<WindowId, std::unique_ptr<GuiWindow> > mWindows;
    std::map<unsigned char, std::unique_ptr<Menu> > mMenus;
    Menu *mViewMenu = nullptr;
};

#endif //OPENGL_GUI_RENDERER_H
