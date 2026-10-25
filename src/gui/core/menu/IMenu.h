#ifndef OPENGL_GUI_IMENU_H
#define OPENGL_GUI_IMENU_H

class IMenu
{
public:
    virtual ~IMenu() = default;

    virtual void render() const = 0;
};


#endif //OPENGL_GUI_IMENU_H
