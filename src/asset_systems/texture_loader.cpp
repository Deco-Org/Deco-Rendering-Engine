/**
 * @file texture_loader.cpp
 * @brief
 */

#include "texture_loader.hpp"
#include <stb_image.h>

TextureLoader::TextureLoader(MTL::Device* metalDevice)
{
    device = metalDevice;
}

TextureLoader::~TextureLoader()
{
    std::vector<MTL::Texture*> textures;
    textures.reserve(uniqueTexturesCount);
    // for (const auto& pair : fileToHandleMap)
    // {
    //     textures.push_back(trackedTextures[pair.second].texture);
    // }
    // for (size_t i = 0; i < uniqueTexturesCount; ++i)
    // {
    //     textures[i]->release();
    // }
    for (TextureHandle i = 0; i < trackedTextures.size(); ++i)
    {
        if (trackedTextures[i].texture)
        {
            trackedTextures[i].texture->release();
            textureToFileMap.erase(trackedTextures[i].texture);
            textureToHandleMap.erase(trackedTextures[i].texture);
            trackedTextures[i].texture = nullptr;
        }
    }
    textureToFileMap.clear();
    textureToHandleMap.clear();
}

TextureLoader::AddedTextureInfo TextureLoader::loadTexture(std::filesystem::path filepath, MTL::PixelFormat pixelFormat, int desiredChannels)
{
    std::string absoluteFileStr = std::filesystem::absolute(filepath).string();
    // If the file is already loaded, increment the uasge count and return the texture
    if (fileToHandleMap.contains(absoluteFileStr) && trackedTextures[fileToHandleMap[absoluteFileStr]].texture != nullptr)
    {
        trackedTextures[fileToHandleMap[absoluteFileStr]].useCount += 1;
        const TextureHandle handle = fileToHandleMap[absoluteFileStr];
        return (AddedTextureInfo) {
            .handle = handle,
            .texture = trackedTextures[handle].texture
        };
    }

    int width, height, channels;
    unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, 4);

    MTL::Texture* texture;
    TextureHandle handle;

    if (!pixels)
    {
        texture = nullptr;
        handle = INVALID_TEXTURE_HANDLE;
    }
    else
    {
        // Converting from BGRA to RGBA
        for (int i = 0; i < width * height; i++)
        {
            unsigned char temp = pixels[i * 4 + 0]; // Red
            pixels[i * 4 + 0] = pixels[i * 4 + 2]; // Setting blue to red
            pixels[i * 4 + 2] = temp; // Setting red to what was in blue
        }

        const AddedTextureInfo info = addTexture(
            pixels,
            width,
            height,
            channels,
            desiredChannels,
            pixelFormat
        );
        texture = info.texture;
        handle = info.handle;
        fileToHandleMap[absoluteFileStr] = info.handle;
        textureToFileMap[texture] = absoluteFileStr;
    }
    stbi_image_free(pixels);
    return (AddedTextureInfo) {
        .handle = handle,
        .texture = texture
    };
}

TextureLoader::AddedTextureInfo TextureLoader::loadPackedTexture(
    TextureHandle textureHandle0,
    TextureHandle textureHandle1,
    TextureHandle textureHandle2,
    TextureHandle textureHandle3,
    uint8_t desiredNumberOfChannels)
{
    // TODO: Replace this cpu side texture packing with a compute shader
    // TODO: Add support for other pixel formats
    const uint8_t bytesPerPixel = desiredNumberOfChannels * 1;

    AddedTextureInfo addedTexture = {
        .handle = INVALID_TEXTURE_HANDLE,
        .texture = nullptr
    };

    MTL::Texture* texture0 = nullptr;
    MTL::Texture* texture1 = nullptr;
    MTL::Texture* texture2 = nullptr;
    MTL::Texture* texture3 = nullptr;

    size_t texture0Width = 0;
    size_t texture0Height = 0;
    size_t texture1Width = 0;
    size_t texture1Height = 0;
    size_t texture2Width = 0;
    size_t texture2Height = 0;
    size_t texture3Width = 0;
    size_t texture3Height = 0;

    uint8_t bytesPerPixelOfTextures[4] = {0, 0, 0, 0};

    // If texture0 is nullptr, no texture should be added
    if (textureHandle0 != INVALID_TEXTURE_HANDLE && textureHandle0 < trackedTextures.size() && trackedTextures[textureHandle0].texture != nullptr)
    {
        if (!trackedTextures[textureHandle0].isOnRenderThread())
        {
            texture0 = trackedTextures[textureHandle0].texture;
            trackedTextures[textureHandle0].useCount += 1;
            bytesPerPixelOfTextures[0] = getNumberOfBytesPerPixelFromPixelFormat(texture0->pixelFormat());

            if (textureHandle1 != INVALID_TEXTURE_HANDLE && textureHandle1 < trackedTextures.size() && trackedTextures[textureHandle1].texture != nullptr)
            {
                // If there is texture1, add it
                // The first channel should be used from texture 1
                if (trackedTextures[textureHandle1].isOnRenderThread())
                {
                    // TODO: Come up with a solution for this
                }
                else
                {
                    texture1 = trackedTextures[textureHandle1].texture;
                    texture1Width = texture1->width();
                    texture1Height = texture1->height();
                    bytesPerPixelOfTextures[1] = getNumberOfBytesPerPixelFromPixelFormat(texture1->pixelFormat());
                }
                trackedTextures[textureHandle1].useCount += 1;
            }
            if (textureHandle2 != INVALID_TEXTURE_HANDLE && textureHandle2 < trackedTextures.size() && trackedTextures[textureHandle2].texture != nullptr)
            {
                if (!trackedTextures[textureHandle2].isOnRenderThread())
                {
                    texture2 = trackedTextures[textureHandle2].texture;
                    texture2Width = texture2->width();
                    texture2Height = texture2->height();
                    bytesPerPixelOfTextures[2] = getNumberOfBytesPerPixelFromPixelFormat(texture2->pixelFormat());
                }
                trackedTextures[textureHandle2].useCount += 1;
            }
            if (textureHandle3 != INVALID_TEXTURE_HANDLE && textureHandle3 < trackedTextures.size() && trackedTextures[textureHandle3].texture != nullptr)
            {
                if (!trackedTextures[textureHandle3].isOnRenderThread())
                {
                    texture3 = trackedTextures[textureHandle3].texture;
                    texture3Width = texture3->width();
                    texture3Height = texture3->height();
                    bytesPerPixelOfTextures[3] = getNumberOfBytesPerPixelFromPixelFormat(texture3->pixelFormat());
                }
                trackedTextures[textureHandle3].useCount += 1;
            }

            // Getting the raw data from each of the textures
            texture0Width = texture0->width();
            texture0Height = texture0->height();

            uint8_t* texture0Data = new uint8_t[texture0Width * texture0Height];
            uint8_t* texture1Data = new uint8_t[texture1Width * texture1Height];
            uint8_t* texture2Data = new uint8_t[texture2Width * texture2Height];
            uint8_t* texture3Data = new uint8_t[texture3Width * texture3Height];

            getTexturePixels(texture0Data, texture0, texture0Width, texture0Height);
            getTexturePixels(texture1Data, texture1, texture1Width, texture1Height);
            getTexturePixels(texture2Data, texture2, texture2Width, texture2Height);
            getTexturePixels(texture3Data, texture3, texture3Width, texture3Height);

            uint8_t* pixels = new uint8_t[texture0Width * texture0Height * desiredNumberOfChannels];

            for (size_t i = 0; i < texture0Width; ++i)
            {
                for (size_t j = 0; j < texture0Height; ++j)
                {

                    if (i < texture0Width && j < texture0Height)
                        pixels[(i * texture0Width + j) * bytesPerPixel + 0] = texture0Data[(i * texture0Width + j) * bytesPerPixelOfTextures[0]];
                    else
                        pixels[(i * texture0Width + j) * bytesPerPixel + 0] = 0;


                    if (i < texture1Width && j < texture1Height)
                        pixels[(i * texture1Width + j) * bytesPerPixel + 1] = texture1Data[(i * texture1Width + j) * bytesPerPixelOfTextures[1]];
                    else
                        pixels[(i * texture1Width + j) * bytesPerPixel + 1] = 0;


                    if (i < texture2Width && j < texture2Height)
                        pixels[(i * texture2Width + j) * bytesPerPixel + 2] = texture2Data[(i * texture2Width + j) * bytesPerPixelOfTextures[2]];
                    else
                        pixels[(i * texture2Width + j) * bytesPerPixel + 2] = 0;


                    if (i < texture3Width && j < texture3Height)
                        pixels[(i * texture3Width + j) * bytesPerPixel + 3] = texture3Data[(i * texture3Width + j) * bytesPerPixelOfTextures[3]];
                    else
                        pixels[(i * texture3Width + j) * bytesPerPixel + 3] = 255u;
                }
            }

            MTL::PixelFormat pixelFormat;
            switch (desiredNumberOfChannels)
            {
                case 3:
                case 4:
                    pixelFormat = MTL::PixelFormat::PixelFormatRGBA8Unorm;
                    break;

                default:
                    pixelFormat = MTL::PixelFormat::PixelFormatRGBA8Unorm;
            };

            addedTexture = addTexture(
                pixels,
                texture0Width,
                texture0Height,
                desiredNumberOfChannels,
                desiredNumberOfChannels,
                pixelFormat
            );

            trackedTextures[addedTexture.handle].component0 = textureHandle0;
            trackedTextures[addedTexture.handle].component1 = textureHandle1;
            trackedTextures[addedTexture.handle].component2 = textureHandle2;
            trackedTextures[addedTexture.handle].component3 = textureHandle3;
            
            delete[] texture0Data;
            delete[] texture1Data;
            delete[] texture2Data;
            delete[] texture3Data;
            delete[] pixels;
        }
    }
    return addedTexture;
}

TextureLoader::AddedTextureInfo TextureLoader::addTexture(uint8_t* pixels, int width, int height, int channelsInImage, int desiredChannels, MTL::PixelFormat pixelFormat)
{
    // TODO: Find a way to use MTL::StorageModePrivate (probably involves using blit commands)
    MTL::Texture* texture = nullptr;
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setTextureType(MTL::TextureType2D);
    textureDescriptor->setPixelFormat(pixelFormat);
    textureDescriptor->setWidth(width);
    textureDescriptor->setHeight(height);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
    textureDescriptor->setStorageMode(MTL::StorageModeShared);

    texture = device->newTexture(textureDescriptor);
    textureDescriptor->release();

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
    NS::UInteger bytesPerRow = desiredChannels * width;

    texture->replaceRegion(region, 0, pixels, bytesPerRow);

    // Mapping
    const TextureHandle handle = getNNextFreeHandles(1)[0];
    trackedTextures[handle] = {
        .texture = texture,
        .useCount = 1
    };
    textureToHandleMap[texture] = handle;
    uniqueTexturesCount += 1;
    AddedTextureInfo info = {
        .texture = texture,
        .handle = handle
    };
    return info;
}

void TextureLoader::unloadTexture(MTL::Texture* texture)
{
    if (textureToHandleMap.contains(texture))
    {
        unloadTexture(textureToHandleMap[texture]);
    }
}

void TextureLoader::unloadTexture(TextureHandle handle)
{
    trackedTextures[handle].useCount -= 1;
    // fileToHandleMap[trackedTextures[handle].fileStr];
    MTL::Texture* texture = trackedTextures[handle].texture;
    if (texture && trackedTextures[handle].useCount < 1)
    {
        // Unloading all the components
        if (trackedTextures[handle].component0 != INVALID_TEXTURE_HANDLE)
            unloadTexture(trackedTextures[handle].component0);
        if (trackedTextures[handle].component1 != INVALID_TEXTURE_HANDLE)
            unloadTexture(trackedTextures[handle].component1);
        if (trackedTextures[handle].component2 != INVALID_TEXTURE_HANDLE)
            unloadTexture(trackedTextures[handle].component2);
        if (trackedTextures[handle].component3 != INVALID_TEXTURE_HANDLE)
            unloadTexture(trackedTextures[handle].component3);

        // Unloading the texture itself
        if (textureToFileMap.contains(texture))
        {
            std::string filename = textureToFileMap[texture];
            textureToFileMap.erase(texture);
            fileToHandleMap.erase(filename);
        }
        textureToHandleMap.erase(texture);
        texture->release();
        trackedTextures[handle].texture = nullptr;
        freeHandles.push_back(handle);
        uniqueTexturesCount -= 1;
    }
}

uint32_t TextureLoader::getUseCount(MTL::Texture* texture) const
{
    uint32_t useCount = 0;
    if (texture)
    {
        if (textureToFileMap.contains(texture))
        {
            std::string fileStr = textureToFileMap.at(texture);
            if (fileToHandleMap.contains(fileStr))
            {
                useCount = trackedTextures[fileToHandleMap.at(fileStr)].useCount;
            }
        }
        else if (textureToHandleMap.contains(texture))
        {
            TextureHandle handle = textureToHandleMap.at(texture);
            useCount = trackedTextures[handle].useCount;
        }
    }
    return useCount;
}

uint32_t TextureLoader::getUseCount(TextureHandle handle) const
{
    uint32_t useCount = 0;
    if (handle < trackedTextures.size() && trackedTextures[handle].texture)
    {
        useCount = trackedTextures[handle].useCount;
    }
    return useCount;
}

void TextureLoader::markTextureAsUsedByRenderThread(MTL::Texture* texture, bool set)
{
    TextureHandle handle = getHandle(texture);
    if (handle != INVALID_TEXTURE_HANDLE)
        trackedTextures[handle].markAsSentToRenderThread(true);
}

bool TextureLoader::textureIsOnRenderThread(MTL::Texture* texture) const
{
    TextureHandle handle = getHandle(texture);
    if (handle == INVALID_TEXTURE_HANDLE)
        return false;
    
    return trackedTextures[handle].isOnRenderThread();
}

TextureLoader::TextureHandle TextureLoader::getNextFreeHandle()
{
    if (freeHandles.size() == 0)
    {
        trackedTextures.resize(trackedTextures.size() + 1);
        return trackedTextures.size();
    }
    else
    {
        TextureHandle handle = freeHandles[freeHandles.size() - 1];
        freeHandles.pop_back();
        return freeHandles[handle];
    }
}

std::vector<TextureLoader::TextureHandle> TextureLoader::getNNextFreeHandles(size_t n)
{
    std::vector<TextureHandle> handles;
    handles.reserve(n);

    if (freeHandles.size() == 0)
    {
        // Appending n empty textures to the end of the trackedTextures std::vector
        const size_t numberOfTrackedTextures = trackedTextures.size();
        trackedTextures.resize(numberOfTrackedTextures + n);
        for (size_t i = 0; i < n; ++i)
        {
            handles.push_back(numberOfTrackedTextures + i);
        }
    }
    else
    {
        // Getting all the free handles
        const size_t numberOfFreeHandles = freeHandles.size();
        const size_t numberOfFreeHandlesToTake = std::min(numberOfFreeHandles, n);
        const size_t numberOfRemainingHandles = numberOfFreeHandles - numberOfFreeHandlesToTake;
        for (size_t i = 0; i < numberOfFreeHandlesToTake; ++i)
        {
            // Getting the free handles from the back (getting from the front would be an O(n) operation)
            handles.push_back(freeHandles[freeHandles.size() - 1 - i]);
            freeHandles.pop_back();
        }

        // Getting the remaining handles
        const size_t numberOfOldTextures = trackedTextures.size();
        trackedTextures.resize(trackedTextures.size() + numberOfRemainingHandles);
        for (size_t i = 0; i < numberOfRemainingHandles; ++i)
        {
            handles.push_back(numberOfOldTextures + i);
        }
    }

    return handles;
}

void TextureLoader::getTexturePixels(uint8_t* pixels, MTL::Texture* texture, size_t width, size_t height)
{
    if (!texture || !pixels)
        return;

    const uint8_t bytesPerPixel = getNumberOfBytesPerPixelFromPixelFormat(texture->pixelFormat());
    texture->getBytes(
        pixels,
        bytesPerPixel * width,
        bytesPerPixel * width * height,
        MTL::Region::Make2D(0, 0, width, height),
        0,
        0
    );
}

uint8_t TextureLoader::getNumberOfBytesPerPixelFromPixelFormat(MTL::PixelFormat pixelFormat) const
{
    uint8_t bytesPerPixel = -1U;
    switch (pixelFormat)
    {
        using enum MTL::PixelFormat;
        case PixelFormatA8Unorm:
        case PixelFormatR8Unorm:
        case PixelFormatR8Unorm_sRGB:
        case PixelFormatR8Snorm:
        case PixelFormatR8Uint:
        case PixelFormatR8Sint:
            bytesPerPixel = 1;
            break;

        case PixelFormatR16Unorm:
        case PixelFormatR16Snorm:
        case PixelFormatR16Uint:
        case PixelFormatR16Sint:
        case PixelFormatR16Float:
        case PixelFormatRG8Unorm:
        case PixelFormatRG8Unorm_sRGB:
        case PixelFormatRG8Snorm:
        case PixelFormatRG8Uint:
        case PixelFormatRG8Sint:
            bytesPerPixel = 2;
            break;

        case PixelFormatR32Uint:
        case PixelFormatR32Sint:
        case PixelFormatR32Float:
        case PixelFormatRG16Unorm:
        case PixelFormatRG16Snorm:
        case PixelFormatRG16Uint:
        case PixelFormatRG16Sint:
        case PixelFormatRG16Float:
        case PixelFormatRGBA8Unorm:
        case PixelFormatRGBA8Unorm_sRGB:
        case PixelFormatRGBA8Snorm:
        case PixelFormatRGBA8Uint:
        case PixelFormatRGBA8Sint:
        case PixelFormatBGRA8Unorm:
        case PixelFormatBGRA8Unorm_sRGB:
            bytesPerPixel = 4;
            break;

        case PixelFormatRG32Uint:
        case PixelFormatRG32Sint:
        case PixelFormatRG32Float:
        case PixelFormatRGBA16Unorm:
        case PixelFormatRGBA16Snorm:
        case PixelFormatRGBA16Uint:
        case PixelFormatRGBA16Sint:
        case PixelFormatRGBA16Float:
            bytesPerPixel = 8;
            break;
            
        case PixelFormatRGBA32Uint:
        case PixelFormatRGBA32Sint:
        case PixelFormatRGBA32Float:
            bytesPerPixel = 16;
            break;
    }
    return bytesPerPixel;
}