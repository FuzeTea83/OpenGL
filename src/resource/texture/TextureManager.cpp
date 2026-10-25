#include "TextureManager.h"

#include <iostream>
#include <stb_image.h>
#include <stdexcept>

#include "core/Engine.h"
#include "utils/OpenGLUtils.h"
#include "utils/concurrency/CompletableFuture.h"

const Texture &TextureManager::getTexture(const TextureId id)
{
    std::lock_guard lock(mMutex);
    if (id > mTextures.size() || id <= 0) { throw std::runtime_error("Invalid texture ID"); }
    return mTextures[id - 1];
}

CompletableFuture<TextureId> TextureManager::loadTextureAsync(const std::filesystem::path &path,
                                                              const bool generateMipMaps, const bool hdr)
{
    return CompletableFuture<TextureId>::runAsync(
        [this, path = std::filesystem::absolute(path), generateMipMaps, hdr]
        {
            const std::size_t hash = std::hash<std::filesystem::path>{}(path);

            {
                std::lock_guard lock(mMutex);
                const auto it = mLookup.find(hash);
                if (it != mLookup.end()) { return it->second; }
            }

            std::cout << "Loading texture: " << std::filesystem::absolute(path) << std::endl;

            StbImage image;
            if (!loadImage(image, path, true, hdr)) { return INVALID_TEXTURE_ID; }



            const GLuint glId = mEngine.runOnGlThread<GLuint>([image, generateMipMaps]
            {
                return createTexture(image, generateMipMaps);
            });

            std::lock_guard lock(mMutex);
            const TextureId id = static_cast<uint32_t>(mTextures.size()) + 1;
            mLookup.emplace(hash, id);
            mTextures.push_back({.glId = glId, .cubeMap = false, .transparent = image.isTransparent});
            return id;
        }, mEngine.getEngineExecutor());
}


TextureId TextureManager::loadTexture(const std::filesystem::path &path, const bool generateMipMaps, const bool hdr)
{
    assertGlThread();

    const std::size_t hash = std::hash<std::filesystem::path>{}(path);

    {
        std::lock_guard lock(mMutex);
        const auto it = mLookup.find(hash);
        if (it != mLookup.end()) { return it->second; }
    }

    std::cout << "Loading texture: " << std::filesystem::absolute(path) << std::endl;

    StbImage image;
    if (!loadImage(image, path, true, hdr)) { return INVALID_TEXTURE_ID; }

    const GLuint glId = createTexture(image, generateMipMaps);

    std::lock_guard lock(mMutex);
    const TextureId id = static_cast<uint32_t>(mTextures.size()) + 1;
    mLookup.emplace(hash, id);
    mTextures.push_back({.glId = glId, .cubeMap = false, .transparent = image.isTransparent});
    return id;
}


CompletableFuture<TextureId> TextureManager::loadTextureFromMemoryAsync(const void *data, const std::size_t size,
                                                                        const bool generateMipMaps,
                                                                        const bool hdr)
{
    return CompletableFuture<TextureId>::runAsync(
        [this, data, size, generateMipMaps, hdr]
        {
            StbImage image;

            if (!loadImageFromMemory(image, static_cast<const unsigned char *>(data), size, true, hdr))
            {
                return INVALID_TEXTURE_ID;
            }

            const std::size_t h = hash(static_cast<const unsigned char *>(image.data),
                                  image.width * image.height * image.channels * (hdr ? 4 : 1));

            {
                std::lock_guard lock(mMutex);
                const auto it = mLookup.find(h);
                if (it != mLookup.end())
                {
                    stbi_image_free(image.data);
                    return it->second;
                }
            }

            const GLuint glId = CompletableFuture<GLuint>::runAsync(
                [image, generateMipMaps] { return createTexture(image, generateMipMaps); },
                mEngine.getGlExecutor()
            ).get();

            std::lock_guard lock(mMutex);
            const TextureId id = static_cast<uint32_t>(mTextures.size()) + 1;

            mLookup.emplace(h, id);
            mTextures.push_back({.glId = glId, .cubeMap = false, .transparent = image.isTransparent});
            return id;
        }, mEngine.getEngineExecutor());
}

TextureId TextureManager::loadTextureFromMemory(const void *data, const std::size_t size, const bool generateMipMaps,
                                                const bool hdr)
{
    assertGlThread();
    StbImage image;
    if (!loadImageFromMemory(image, static_cast<const unsigned char *>(data), size, true, hdr))
    {
        return INVALID_TEXTURE_ID;
    }
    const std::size_t h = hash(static_cast<const unsigned char *>(image.data),
                          image.width * image.height * image.channels * (hdr ? 4 : 1));

    {
        std::lock_guard lock(mMutex);
        const auto it = mLookup.find(h);
        if (it != mLookup.end())
        {
            stbi_image_free(image.data);
            return it->second;
        }
    }

    const GLuint glId = createTexture(image, generateMipMaps);

    std::lock_guard lock(mMutex);
    const TextureId id = static_cast<uint32_t>(mTextures.size()) + 1;

    mLookup.emplace(h, id);
    mTextures.push_back({.glId = glId, .cubeMap = false, .transparent = image.isTransparent});
    return id;
}

bool TextureManager::loadImage(StbImage &image, const std::filesystem::path &path, const bool flipVertically,
                               const bool hdr)
{
    stbi_set_flip_vertically_on_load_thread(flipVertically);

    image.hdr = hdr;

    if (hdr) { image.data = stbi_loadf(path.string().c_str(), &image.width, &image.height, &image.channels, 0); }
    else { image.data = stbi_load(path.string().c_str(), &image.width, &image.height, &image.channels, 0); }

    if (image.data == nullptr)
    {
        std::cerr << "Failed to load image: " << path << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }

    switch (image.channels)
    {
        case 1: image.internalFormat = hdr ? GL_R16F : GL_RED;
            image.dataFormat = GL_RED;
            break;
        case 2: image.internalFormat = GL_RG;
            image.dataFormat = GL_RG;
            image.isTransparent = true;
            break;
        case 4: image.internalFormat = hdr ? GL_RGBA16F : GL_RGBA;
            image.dataFormat = GL_RGBA;
            image.isTransparent = true;
            break;
        default: image.internalFormat = hdr ? GL_RGB16F : GL_RGB;
            image.dataFormat = GL_RGB;
            break;
    }

    return true;
}

bool TextureManager::loadImageFromMemory(StbImage &image, const unsigned char *data, const std::size_t size,
                                         const bool flipVertically, const bool hdr)
{
    stbi_set_flip_vertically_on_load(flipVertically);
    image.hdr = hdr;

    if (hdr)
    {
        image.data = stbi_loadf_from_memory(data, static_cast<int>(size), &image.width, &image.height, &image.channels,
                                            0);
    }
    else
    {
        image.data = stbi_load_from_memory(data, static_cast<int>(size), &image.width, &image.height, &image.channels,
                                           0);
    }

    if (image.data == nullptr)
    {
        std::cerr << "Failed to load image from memory." << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }


    switch (image.channels)
    {
        case 1: image.internalFormat = hdr ? GL_R16F : GL_RED;
            image.dataFormat = GL_RED;
            break;
        case 2: image.internalFormat = GL_RG;
            image.dataFormat = GL_RG;
            image.isTransparent = true;
            break;
        case 4: image.internalFormat = hdr ? GL_RGBA16F : GL_RGBA;
            image.dataFormat = GL_RGBA;
            image.isTransparent = true;
            break;
        default: image.internalFormat = hdr ? GL_RGB16F : GL_RGB;
            image.dataFormat = GL_RGB;
            break;
    }

    return true;
}


std::size_t TextureManager::hash(const unsigned char *data, const std::size_t size)
{
    std::size_t h = 14695981039346656037ULL;
    for (std::size_t i = 0; i < size; ++i)
    {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}


GLuint TextureManager::createTexture(const StbImage &image, const bool generateMipMaps)
{
    assertGlThread();
    GLuint glId;
    glGenTextures(1, &glId);
    glBindTexture(GL_TEXTURE_2D, glId);

    glTexImage2D(GL_TEXTURE_2D, 0, image.internalFormat, image.width, image.height, 0, image.dataFormat,
                 image.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, image.data);
    stbi_image_free(image.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (image.channels == 1)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    }
    else if (image.channels == 2)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, generateMipMaps ? GL_LINEAR : GL_NEAREST);
    if (generateMipMaps) { glGenerateMipmap(GL_TEXTURE_2D); }

    glBindTexture(GL_TEXTURE_2D, 0);
    return glId;
}
