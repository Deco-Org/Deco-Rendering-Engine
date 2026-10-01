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

class TextureLoader
{
    public:
    using TextureHandle = size_t;
    static constexpr TextureHandle INVALID_TEXTURE_HANDLE = (TextureHandle)(-1);

    struct TrackedTexture
    {
        bool hasParents() const;
        bool isOnRenderThread() const;
        void markAsSentToRenderThread(bool sent = true);

        MTL::Texture* texture = nullptr;
        uint32_t useCount = 0;
        MTL::PixelFormat pixelFormat;

        TextureLoader::TextureHandle component0 = TextureLoader::INVALID_TEXTURE_HANDLE;
        TextureLoader::TextureHandle component1 = TextureLoader::INVALID_TEXTURE_HANDLE;
        TextureLoader::TextureHandle component2 = TextureLoader::INVALID_TEXTURE_HANDLE;
        TextureLoader::TextureHandle component3 = TextureLoader::INVALID_TEXTURE_HANDLE;

        // Done so that TrackedTexture remains a non-aggregate data type
        struct
        {
            bool accessedByRenderThread = false;
        } _private;
    };

    struct AddedTextureInfo
    {
        MTL::Texture* texture = nullptr;
        TextureHandle handle = INVALID_TEXTURE_HANDLE;
    };

    TextureLoader(MTL::Device* metalDevice = nullptr);
    ~TextureLoader();

    AddedTextureInfo loadTexture(
        std::filesystem::path filepath,
        MTL::PixelFormat pixelFormat = MTL::PixelFormat::PixelFormatBGRA8Unorm,
        int desiredChannels = 4
    );

    AddedTextureInfo loadPackedTexture(
        TextureHandle textureHandle0,
        TextureHandle textureHandle1,
        TextureHandle textureHandle2,
        TextureHandle textureHandle3 = INVALID_TEXTURE_HANDLE,
        uint8_t desiredNumberOfChannels = 4);

    AddedTextureInfo addTexture(
        uint8_t* pixels, 
        int width, 
        int height, 
        int channelsInImage, 
        int desiredChannels,
        MTL::PixelFormat pixelFormat);

    void unloadTexture(MTL::Texture* texture);
    void unloadTexture(TextureHandle handle);
    void forceUnloadTexture(MTL::Texture* texture);

    uint32_t getUseCount(MTL::Texture* texture) const;
    uint32_t getUseCount(TextureHandle handle) const;
    uint32_t getUseCount(std::filesystem::path file) const;

    MTL::Texture* getTexture(TextureHandle handle) const;
    TextureHandle getHandle(MTL::Texture* texture) const;
    void markTextureAsUsedByRenderThread(MTL::Texture* texture, bool set = true);
    bool textureIsOnRenderThread(MTL::Texture* texture) const;
    
    std::vector<TrackedTexture> trackedTextures;

    private:
    TextureHandle getNextFreeHandle();
    std::vector<TextureHandle> getNNextFreeHandles(size_t n);
    void getTexturePixels(uint8_t* pixels, MTL::Texture* texture, size_t width, size_t height);
    uint8_t getNumberOfBytesPerPixelFromPixelFormat(MTL::PixelFormat pixelFormat) const;

    MTL::Device* device;
    size_t uniqueTexturesCount = 0;
    std::unordered_map<std::string, TextureHandle> fileToHandleMap;
    std::unordered_map<MTL::Texture*, std::string> textureToFileMap;
    // Maps textures to handles
    // TODO: Come up with a better way of doing this.
    std::unordered_map<MTL::Texture*, TextureHandle> textureToHandleMap;

    std::vector<TextureHandle> freeHandles;
};

// Inlines

inline MTL::Texture* TextureLoader::getTexture(TextureLoader::TextureHandle handle) const
{
    return trackedTextures[handle].texture;
}

inline TextureLoader::TextureHandle TextureLoader::getHandle(MTL::Texture* texture) const
{
    const auto iterator = textureToHandleMap.find(texture);
    return (iterator != textureToHandleMap.end()) ? iterator->second : INVALID_TEXTURE_HANDLE;
}

inline bool TextureLoader::TrackedTexture::hasParents() const
{
    return (
        component0 == INVALID_TEXTURE_HANDLE &&
        component1 == INVALID_TEXTURE_HANDLE &&
        component2 == INVALID_TEXTURE_HANDLE &&
        component3 == INVALID_TEXTURE_HANDLE
    );
}

// TODO: Come up with better implementation. For now, this is just a wrapper.
inline bool TextureLoader::TrackedTexture::isOnRenderThread() const
{
    return _private.accessedByRenderThread;
}

// TODO: Come up with better implementation. For now, this is just a wrapper.
inline void TextureLoader::TrackedTexture::markAsSentToRenderThread(bool sent)
{
    _private.accessedByRenderThread = sent;
}