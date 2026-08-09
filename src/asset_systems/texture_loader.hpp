/**
 * @file texture_loader.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include <filesystem>
#include "core_engine_types.h"
#include "tools/synchronized_buffer.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"

struct TrackedTexture
{
    MTL::Texture* texture = nullptr;
    uint32_t useCount = 0;
};

class TextureLoader
{
    public:
    using TextureHandle = size_t;
    static constexpr TextureHandle INVALID_TEXTURE_HANDLE = (TextureHandle)(-1);

    struct AddedTextureInfo
    {
        MTL::Texture* texture = nullptr;
        TextureHandle handle = INVALID_TEXTURE_HANDLE;
    };

    TextureLoader(MTL::Device* metalDevice = nullptr);
    ~TextureLoader();

    MTL::Texture* loadTexture(
        std::filesystem::path filepath,
        MTL::PixelFormat pixelFormat = MTL::PixelFormat::PixelFormatBGRA8Unorm,
        int desiredChannels = 4
    );

    AddedTextureInfo addTexture(
        uint8_t* pixels, 
        int width, 
        int height, 
        int channelsInImage, 
        int desiredChannels,
        MTL::PixelFormat pixelFormat);

    void unloadTexture(MTL::Texture* texture);
    void forceUnloadTexture(MTL::Texture* texture);

    uint32_t getUseCount(MTL::Texture* texture) const;
    uint32_t getUseCount(TextureHandle handle) const;
    uint32_t getUseCount(std::filesystem::path file) const;

    private:
    TextureHandle getNextFreeHandle();
    std::vector<TextureHandle> getNNextFreeHandles(size_t n);

    MTL::Device* device;
    size_t uniqueTexturesCount = 0;
    std::unordered_map<std::string, TextureHandle> fileToHandleMap;
    std::unordered_map<MTL::Texture*, std::string> textureToFileMap;
    // Maps textures to handles
    // TODO: Come up with a better way of doing this.
    std::unordered_map<MTL::Texture*, TextureHandle> textureToHandleMap;

    std::vector<TrackedTexture> trackedTextures;
    std::vector<TextureHandle> freeHandles;
};