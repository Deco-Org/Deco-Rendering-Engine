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
    for (const auto& pair : fileToHandleMap)
    {
        textures.push_back(trackedTextures[pair.second].texture);
    }
    for (size_t i = 0; i < uniqueTexturesCount; ++i)
    {
        textures[i]->release();
    }
}

MTL::Texture* TextureLoader::loadTexture(std::filesystem::path filepath, MTL::PixelFormat pixelFormat, int desiredChannels)
{
    std::string absoluteFileStr = std::filesystem::absolute(filepath).string();
    // If the file is already loaded, increment the uasge count and return the texture
    if (fileToHandleMap.contains(absoluteFileStr) && trackedTextures[fileToHandleMap[absoluteFileStr]].texture != nullptr)
    {
        trackedTextures[fileToHandleMap[absoluteFileStr]].useCount += 1;
        return trackedTextures[fileToHandleMap[absoluteFileStr]].texture;
    }

    int width, height, channels;
    unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, 4);

    MTL::Texture* texture;

    if (!pixels)
    {
        texture = nullptr;
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
        fileToHandleMap[absoluteFileStr] = info.handle;
        textureToFileMap[texture] = absoluteFileStr;
    }
    stbi_image_free(pixels);
    return texture;
}

TextureLoader::AddedTextureInfo TextureLoader::addTexture(uint8_t* pixels, int width, int height, int channelsInImage, int desiredChannels, MTL::PixelFormat pixelFormat)
{
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
    if (textureToFileMap.contains(texture))
    {
        std::string fileStr = textureToFileMap.at(texture);
        if (fileToHandleMap.contains(fileStr))
        {
            const TextureHandle handle = fileToHandleMap[fileStr];
            trackedTextures[handle].useCount -= 1;

            // If there are no remaining references to the texture, unload it
            if (trackedTextures[handle].useCount < 1)
            {
                trackedTextures[handle].texture->release();
                trackedTextures[handle].texture = nullptr;
                freeHandles.push_back(handle);
                fileToHandleMap.erase(fileStr);
                uniqueTexturesCount -= 1;
            }
        }
    }
    else if (textureToHandleMap.contains(texture))
    {
        const TextureHandle handle = textureToHandleMap.at(texture);
        trackedTextures[handle].useCount -= 1;

        // If there are no remaining references to the texture, unload it
        if (trackedTextures[handle].useCount < 1)
        {
            trackedTextures[handle].texture->release();
            trackedTextures[handle].texture = nullptr;
            freeHandles.push_back(handle);
            uniqueTexturesCount -= 1;
        }
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