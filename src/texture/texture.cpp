// 
// texture.cpp
//
// Created on 19 June 2026
// 

#include "texture.hpp"

Texture::Texture(MTL::Device* device, MTL::PixelFormat pixelFmt)
{
    metalDevice = device;
    pixelFormat = pixelFmt;
}

MTL::Texture* Texture::loadTexture(char* filePath)
{
    // Checking to make sure the file actually exists
    int width, height, channels;
    unsigned char* pixels = stbi_load(filePath, &width, &height, &channels, NUMBER_OF_CHANNELS);
    
    if (pixels == nullptr)
    {
        std::string errorMessage = "Could not load file: " + std::string(stbi_failure_reason());
        throw std::runtime_error(errorMessage);
    }

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
    
    MTL::Texture* texture = metalDevice->newTexture(textureDescriptor);
    textureDescriptor->release();

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
    NS::UInteger bytesPerRow = NUMBER_OF_CHANNELS * width; // 4 channels, so 4 bytes

    texture->replaceRegion(region, 0, pixels, bytesPerRow);

    stbi_image_free(pixels);
    return texture;
}