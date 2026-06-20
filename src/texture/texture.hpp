// 
// texture.hpp
//
// Created on 19 June 2026
// 

#pragma once
#include <Metal/Metal.hpp>
#include <stb_image.h>
#include <vector>

class Texture
{
    public:
    Texture(MTL::Device* device, MTL::PixelFormat pixelFormat = MTL::PixelFormat::PixelFormatBGRA8Unorm);

    MTL::Texture* loadTexture(char* filePath);

    private:
    static constexpr uint8_t NUMBER_OF_CHANNELS = 4; // 4 channels in BGRA8Unorm
    MTL::Device* metalDevice;
    MTL::PixelFormat pixelFormat;
};