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
    TextureLoader(MTL::Device* metalDevice = nullptr);
    ~TextureLoader();

    MTL::Texture* loadTexture(
        std::filesystem::path filepath,
        MTL::PixelFormat pixelFormat = MTL::PixelFormat::PixelFormatBGRA8Unorm,
        int desiredChannels = 4
    );

    void unloadTexture(MTL::Texture* texture);
    void forceUnloadTexture(MTL::Texture* texture);

    uint32_t getUseCount(MTL::Texture* texture) const;
    uint32_t getUseCount(std::filesystem::path file) const;

    private:
    MTL::Device* device;
    size_t uniqueTexturesCount = 0;
    std::unordered_map<std::string, TrackedTexture> fileToTextureMap;
    std::unordered_map<MTL::Texture*, std::string> textureToFileMap;
};