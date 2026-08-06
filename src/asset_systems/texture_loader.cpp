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
    for (const auto& pair : fileToTextureMap)
    {
        textures.push_back(pair.second.texture);
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
    if (fileToTextureMap.contains(absoluteFileStr) && fileToTextureMap[absoluteFileStr].texture != nullptr)
    {
        fileToTextureMap[absoluteFileStr].useCount += 1;
        return fileToTextureMap[absoluteFileStr].texture;
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
        fileToTextureMap[absoluteFileStr] = {
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
        if (fileToTextureMap.contains(fileStr))
        {
            fileToTextureMap[fileStr].useCount -= 1;

            // If there are no remaining references to the texture, unload it
            if (fileToTextureMap[fileStr].useCount < 1)
            {
                fileToTextureMap[fileStr].texture->release();
                fileToTextureMap.erase(fileStr);
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
        if (fileToTextureMap.contains(fileStr))
        {
            return fileToTextureMap.at(fileStr).useCount;
        }
    }
    return 0;
}