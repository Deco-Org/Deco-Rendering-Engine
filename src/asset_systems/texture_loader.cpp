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
        NS::UInteger bytesPerRow = 4 * width; // 4 channels, so 4 bytes

        texture->replaceRegion(region, 0, pixels, bytesPerRow);

        
        // Mapping
        const TextureHandle index = getNNextFreeHandles(1)[0];
        fileToHandleMap[absoluteFileStr] = index;
        trackedTextures[index] = {
            .texture = texture,
            .useCount = 1
        };
        textureToFileMap[texture] = absoluteFileStr;
        uniqueTexturesCount += 1;
    }
    stbi_image_free(pixels);
    return texture;
}

void TextureLoader::unloadTexture(MTL::Texture* texture)
{
    if (textureToFileMap.contains(texture))
    {
        std::string fileStr = textureToFileMap.at(texture);
        if (fileToHandleMap.contains(fileStr))
        {
            trackedTextures[fileToHandleMap[fileStr]].useCount -= 1;

            // If there are no remaining references to the texture, unload it
            if (trackedTextures[fileToHandleMap[fileStr]].useCount < 1)
            {
                trackedTextures[fileToHandleMap[fileStr]].texture->release();
                trackedTextures[fileToHandleMap[fileStr]].texture = nullptr;
                freeHandles.push_back(fileToHandleMap[fileStr]);
                fileToHandleMap.erase(fileStr);
                uniqueTexturesCount -= 1;
            }
        }
    }
}

uint32_t TextureLoader::getUseCount(MTL::Texture* texture) const
{
    if (texture && textureToFileMap.contains(texture))
    {
        std::string fileStr = textureToFileMap.at(texture);
        if (fileToHandleMap.contains(fileStr))
        {
            return trackedTextures[fileToHandleMap.at(fileStr)].useCount;
        }
    }
    return 0;
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