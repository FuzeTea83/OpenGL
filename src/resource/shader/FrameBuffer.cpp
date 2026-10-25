#include "FrameBuffer.h"

#include <iostream>
#include <stdexcept>

FrameBuffer::FrameBuffer(const int width, const int height, const bool hdr, const bool depth)
    : mWidth(width), mHeight(height), mHdr(hdr), mDepth(depth) { create(); }

FrameBuffer::~FrameBuffer() { destroy(); }

FrameBuffer::FrameBuffer(FrameBuffer &&other) noexcept { *this = std::move(other); }

FrameBuffer &FrameBuffer::operator=(FrameBuffer &&other) noexcept
{
    if (this == &other) { return *this; }
    destroy();
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mHdr = other.mHdr;
    mDepth = other.mDepth;
    mFbo = other.mFbo;
    mTexture = other.mTexture;
    mDepthBuffer = other.mDepthBuffer;
    other.mFbo = other.mTexture = other.mDepthBuffer = 0;
    return *this;
}

void FrameBuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glViewport(0, 0, mWidth, mHeight);
}

void FrameBuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void FrameBuffer::resize(const int width, const int height)
{
    if (width <= 0 || height <= 0) { return; }
    mWidth = width;
    mHeight = height;
    destroy();
    create();
}

void FrameBuffer::destroy()
{
    if (mDepthBuffer) { glDeleteRenderbuffers(1, &mDepthBuffer); }
    if (mTexture) { glDeleteTextures(1, &mTexture); }
    if (mFbo)
    {
        glDeleteFramebuffers(1, &mFbo);
        mDepthBuffer = mTexture = mFbo = 0;
    }
}

void FrameBuffer::create()
{
    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, mHdr ? GL_RGBA16F : GL_RGBA8, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);

    if (mDepth)
    {
        glGenRenderbuffers(1, &mDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWidth, mHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        const GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (err != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer Error: " << err << std::endl;
        }

        throw std::runtime_error("Framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
