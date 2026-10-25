#ifndef OPENGL_TEXTURE_MANAGER_H
#define OPENGL_TEXTURE_MANAGER_H

#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Texture.h"
#include "utils/concurrency/CompletableFuture.h"

class Engine;

class TextureManager
{
public:
    explicit TextureManager(Engine &engine) : mEngine(engine) {}

    const Texture &getTexture(TextureId id);

    CompletableFuture<TextureId> loadTextureAsync(const std::filesystem::path &path, bool generateMipMaps, bool hdr);

    TextureId loadTexture(const std::filesystem::path &path, bool generateMipMaps = true, bool hdr = false);

    CompletableFuture<TextureId> loadTextureFromMemoryAsync(const void *data, std::size_t size, bool generateMipMaps,
                                                            bool hdr);

    TextureId loadTextureFromMemory(const void *data, std::size_t size, bool generateMipMaps = true, bool hdr = false);

private:
    static bool loadImage(StbImage &image, const std::filesystem::path &path, bool flipVertically, bool hdr);

    static bool loadImageFromMemory(StbImage &image, const unsigned char *data, std::size_t size, bool flipVertically,
                                    bool hdr);

    static std::size_t hash(const unsigned char *data, std::size_t size);

    static GLuint createTexture(const StbImage &image, bool generateMipMaps);

    Engine &mEngine;
    std::mutex mMutex;
    std::vector<Texture> mTextures;
    std::unordered_map<std::size_t, TextureId> mLookup;
};
#endif //OPENGL_TEXTURE_MANAGER_H
