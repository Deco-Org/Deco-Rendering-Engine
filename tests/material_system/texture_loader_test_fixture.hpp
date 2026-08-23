/**
 * @file texture_loader_test_fixture.hpp
 * @brief
 */

#pragma once
#include "asset_systems/texture_loader.hpp"
#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"

static void packedTextureShouldBeComprisedOfDataFromSourceTextures(
    MTL::Texture* packedTexture,
    MTL::Texture* texture0,
    MTL::Texture* texture1 = nullptr,
    MTL::Texture* texture2 = nullptr,
    MTL::Texture* texture3 = nullptr
)
{
    constexpr uint8_t bytesPerPixel = 1;
    constexpr uint8_t packedTextureBytesPerPixel = 4;
    REQUIRE(packedTexture);
    REQUIRE(texture0);

    // TODO: Add support for other ways of deciding packed texture width (not just using the width of the first texture).
    REQUIRE(texture0->width() == packedTexture->width());

    std::vector<uint8_t> texture0Data;
    std::vector<uint8_t> texture1Data;
    std::vector<uint8_t> texture2Data;
    std::vector<uint8_t> texture3Data;
    std::vector<uint8_t> packedTextureData;

    // MTL::Region region = MTL::Region::Make2D(0, 0, packedTexture->width(), packedTexture->height());
    
    texture0Data.resize(std::max(packedTexture->width(), texture0->width()) * std::max(packedTexture->height(), texture0->height()) * bytesPerPixel);
    texture0->getBytes(
        texture0Data.data(), 
        bytesPerPixel * texture0->width(), 
        bytesPerPixel * texture0->width() * texture0->height(), 
        MTL::Region::Make2D(0, 0, texture0->width(), texture0->height()),
        0,
        0);
    
    if (texture1)
    {
        REQUIRE(texture0->width() == texture1->width());
        const size_t width = texture1->width();
        const size_t height = texture1->height();
        const size_t imageSize = width * height * bytesPerPixel;
        texture1Data.resize(std::max(packedTexture->width(), texture1->width()) * std::max(packedTexture->height(), texture1->height()) * bytesPerPixel);
        texture1->getBytes(
            texture1Data.data(), 
            bytesPerPixel * texture1->width(), 
            imageSize, 
            MTL::Region::Make2D(0, 0, width, height),
            0,
            0);
    }
    if (texture2)
    {
        REQUIRE(texture0->width() == texture2->width());
        const size_t width = texture2->width();
        const size_t height = texture2->height();
        const size_t imageSize = width * height * bytesPerPixel;
        texture2Data.resize(std::max(packedTexture->width(), texture2->width()) * std::max(packedTexture->height(), texture2->height()) * bytesPerPixel);
        texture2->getBytes(
            texture2Data.data(), 
            bytesPerPixel * texture2->width(), 
            imageSize, 
            MTL::Region::Make2D(0, 0, width, height),
            0,
            0);
    }
    if (texture3)
    {
        REQUIRE(texture0->width() == texture3->width());
        const size_t width = texture3->width();
        const size_t height = texture3->height();
        const size_t imageSize = width * height * bytesPerPixel;
        texture3Data.resize(std::max(packedTexture->width(), texture3->width()) * std::max(packedTexture->height(), texture3->height()) * bytesPerPixel);
        texture3->getBytes(
            texture3Data.data(), 
            bytesPerPixel * texture3->width(), 
            imageSize, 
            MTL::Region::Make2D(0, 0, width, height),
            0,
            0);
    }

    // Getting the packed texture data
    packedTextureData.resize(packedTexture->width() * packedTexture->height() * (size_t)(packedTextureBytesPerPixel));
    packedTexture->getBytes(
        packedTextureData.data(),
        packedTextureBytesPerPixel * packedTexture->width(),
        packedTextureBytesPerPixel * packedTexture->width() * packedTexture->height(),
        MTL::Region::Make2D(0, 0, packedTexture->width(), packedTexture->height()),
        0,
        0
    );

    if (texture0Data.size() == 0)
        texture0Data.resize(packedTexture->width() * packedTexture->height() * packedTextureBytesPerPixel);
    if (texture1Data.size() == 0)
        texture1Data.resize(packedTexture->width() * packedTexture->height() * packedTextureBytesPerPixel);
    if (texture2Data.size() == 0)
        texture2Data.resize(packedTexture->width() * packedTexture->height() * packedTextureBytesPerPixel);
    if (texture3Data.size() == 0)
        texture3Data.resize(packedTexture->width() * packedTexture->height() * packedTextureBytesPerPixel);

    CAPTURE(packedTexture->width(), packedTexture->height(), packedTextureData.size());
    CAPTURE(texture1->pixelFormat());

    std::printf("%u\n", packedTextureData[(0 * packedTexture->width() + 0) * packedTextureBytesPerPixel + 0]);
    std::printf("%u\n", packedTextureData[(0 * packedTexture->width() + 1) * packedTextureBytesPerPixel + 0]);
    std::printf("%u\n", packedTextureData[(0 * packedTexture->width() + 2) * packedTextureBytesPerPixel + 0]);
    std::printf("%u\n", packedTextureData[(0 * packedTexture->width() + 3) * packedTextureBytesPerPixel + 0]);

    std::printf("Packed texture:\n");
    for (uint8_t i = packedTexture->width() * (3.0f / 8.0f) - 4; i < packedTexture->width() * (3.0f / 8.0f) + 4; ++i)
    {
        for (uint8_t j = packedTexture->height() * (0.5f) - 4; j < packedTexture->height() * (0.5f) + 4; ++j)
        {
            if (i != 48 || j != 48)
            {
                std::printf("%u\t", packedTextureData[(i * packedTexture->width() + j) * packedTextureBytesPerPixel + 0]);
            }
            else
            {
                std::printf("%ux\t", packedTextureData[(i * packedTexture->width() + j) * packedTextureBytesPerPixel + 0]);
            }
        }
        std::printf("\n");
    }
    std::printf("Texture 0:\n");
    for (uint8_t i = texture0->width() * (3.0f / 8.0f) - 4; i < texture0->width() * (3.0f / 8.0f) + 4; ++i)
    {
        for (uint8_t j = texture0->height() * (0.5f) - 4; j < texture0->height() * (0.5f) + 4; ++j)
        {
            if (i != 48 || j != 48)
            {
                std::printf("%u\t", texture0Data[(i * texture0->width() + j) * bytesPerPixel + 0]);
            }
            else
            {
                std::printf("%ux\t", texture0Data[(i * texture0->width() + j) * bytesPerPixel + 0]);
            }
        }
        std::printf("\n");
    }
    std::printf("Texture 1:\n");
    for (uint8_t i = texture1->width() * (3.0f / 8.0f) - 4; i < texture1->width() * (3.0f / 8.0f) + 4; ++i)
    {
        for (uint8_t j = texture1->height() * (0.5f) - 4; j < texture1->height() * (0.5f) + 4; ++j)
        {
            if (i != 48 || j != 48)
            {
                std::printf("%u\t", texture0Data[(i * texture1->width() + j) * bytesPerPixel + 0]);
            }
            else
            {
                std::printf("%ux\t", texture0Data[(i * texture1->width() + j) * bytesPerPixel + 0]);
            }

        }
        std::printf("\n");
    }

    std::printf("Texture 2:\n");
    for (uint8_t i = texture2->width() * (3.0f / 8.0f) - 4; i < texture2->width() * (3.0f / 8.0f) + 4; ++i)
    {
        for (uint8_t j = texture2->height() * (0.5f) - 4; j < texture2->height() * (0.5f) + 4; ++j)
        {
            if (i != 48 || j != 48)
            {
                std::printf("%u\t", texture0Data[(i * texture2->width() + j) * bytesPerPixel + 0]);
            }
            else
            {
                std::printf("%ux\t", texture0Data[(i * texture2->width() + j) * bytesPerPixel + 0]);
            }
        }
        std::printf("\n");
    }

    std::printf("Texture data at 48, 48: %u, %u, %u \tPacked texture data at 48, 48: %u, %u, %u\n", texture0Data[48 * texture0->width() + 48], texture1Data[48 * texture1->width() + 48], texture2Data[48 * texture2->width() + 48], packedTextureData[(48 * packedTexture->width() + 48) * packedTextureBytesPerPixel], packedTextureData[(48 * packedTexture->width() + 48) * packedTextureBytesPerPixel + 1], packedTextureData[(48 * packedTexture->width() + 48) * packedTextureBytesPerPixel + 2]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 0) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 1) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 2) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 3) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 4) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 5) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 6) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 7) * bytesPerPixel]);
    // std::printf("%u\n", texture0Data[(0 * texture0->width() + 8) * bytesPerPixel]);
    
    for (size_t h = 0; h < packedTexture->width(); ++h)
    {
        for (size_t w = 0; w < packedTexture->height(); ++w)
        {
            CAPTURE(w, h);
            CAPTURE(texture0Data.size(), texture1Data.size(), texture2Data.size(), texture3Data.size());
            uint8_t bytes[4] = {
                texture0Data[(w * texture0->width() + h) * bytesPerPixel],
                texture1Data[(w * texture1->width() + h) * bytesPerPixel],
                texture2Data[(w * texture2->width() + h) * bytesPerPixel],
                texture3Data[(w * texture3->width() + h) * bytesPerPixel],
            };

            if (texture0 && w < texture0->width() && h < texture0->height())
                REQUIRE(packedTextureData[(w * packedTexture->width() + h) * packedTextureBytesPerPixel + 0] == bytes[0]);
            if (texture1 && w < texture1->width() && h < texture1->height())
                REQUIRE(packedTextureData[(w * packedTexture->width() + h) * packedTextureBytesPerPixel + 1] == bytes[1]);
            if (texture2 && w < texture2->width() && h < texture2->height())
                REQUIRE(packedTextureData[(w * packedTexture->width() + h) * packedTextureBytesPerPixel + 2] == bytes[2]);
            if (texture3 && w < texture3->width() && h < texture3->height())
                REQUIRE(packedTextureData[(w * packedTexture->width() + h) * packedTextureBytesPerPixel + 3] == bytes[3]);
        }
    }
}