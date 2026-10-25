#ifndef OPENGL_FRAME_BUFFER_H
#define OPENGL_FRAME_BUFFER_H

#include <glad/gl.h>

class FrameBuffer
{
public:
    FrameBuffer() = default;

    FrameBuffer(int width, int height, bool hdr, bool depth);

    ~FrameBuffer();

    FrameBuffer(const FrameBuffer &other) = delete;

    FrameBuffer &operator=(const FrameBuffer &other) = delete;

    FrameBuffer(FrameBuffer &&other) noexcept;

    FrameBuffer &operator=(FrameBuffer &&other) noexcept;

    void bind() const;

    static void unbind();

    void resize(int width, int height);

    GLuint getTexture() const { return mTexture; }

    GLuint getFramebuffer() const { return mFbo; }

    int getWidth() const { return mWidth; }

    int getHeight() const { return mHeight; }

private:
    void destroy();

    void create();

    int mWidth{0};
    int mHeight{0};
    bool mHdr{false};
    bool mDepth{false};
    GLuint mFbo{0};
    GLuint mTexture{0};
    GLuint mDepthBuffer{0};
};
#endif //OPENGL_FRAME_BUFFER_H
