#ifndef OPENGL_CLONABLE_H
#define OPENGL_CLONABLE_H

struct Clonable
{
    virtual ~Clonable() = default;

    virtual Clonable *clone() const = 0;
};

#endif //OPENGL_CLONABLE_H
