
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "asset_systems/texture_loader.hpp"
#include "test_utils.hpp"
#include <stb_image.h>

TEST_CASE("attempting adding a texture that does not exist should return nullptr", "[texture][loading][asset system][add][error handling][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader loader(metalDevice);

    MTL::Texture* texture = loader.loadTexture(
        std::filesystem::path("assets/nonexistent_texture.png")
    );

    REQUIRE(nullptr == texture);

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a texture should increase the use count of the texture", "[texture][loading][asset system][add][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    SECTION("adding a new unique texture should set the use count of the texture to 1")
    {
        MTL::Texture* texture = loader.loadTexture(
            std::filesystem::path("assets/test_cube_texture.png"),
            MTL::PixelFormat::PixelFormatBGRA8Unorm
        );
        REQUIRE(nullptr != texture);
        REQUIRE(1 == loader.getUseCount(texture));
    }

    SECTION("adding a previously loaded texture should increment the use count of the texture by 1")
    {
        MTL::Texture* texture1 = loader.loadTexture(
            std::filesystem::path("assets/test_cube_texture.png"),
            MTL::PixelFormat::PixelFormatBGRA8Unorm
        );

        REQUIRE(nullptr != texture1);
        REQUIRE(1 == loader.getUseCount(texture1));

        MTL::Texture* texture2 = loader.loadTexture(
            std::filesystem::path("assets/test_cube_texture.png"),
            MTL::PixelFormat::PixelFormatBGRA8Unorm
        );

        REQUIRE(nullptr != texture2);
        REQUIRE(2 == loader.getUseCount(texture2));
        REQUIRE(2 == loader.getUseCount(texture1));
        REQUIRE(texture1 == texture2);
    }
    
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("removing a texture should decrease the use count of the texture", "[texture][loading][asset system][remove][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    MTL::Texture* texture = loader.loadTexture(
        std::filesystem::path("assets/test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatBGRA8Unorm
    );

    REQUIRE(nullptr != texture);
    REQUIRE(1 == loader.getUseCount(texture));

    loader.unloadTexture(texture);

    REQUIRE(0 == loader.getUseCount(texture));

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("textures should be able to be added using raw data", "[texture][asset system][add][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    std::string filepath = std::filesystem::path("assets/test_cube_texture.png").string();
    const int desiredChannels = 4;
    int width, height, channels;

    uint8_t* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, desiredChannels);
    REQUIRE(nullptr != pixels);

    TextureLoader::AddedTextureInfo info = loader.addTexture(
        pixels,
        width,
        height,
        channels,
        desiredChannels,
        MTL::PixelFormat::PixelFormatBGRA8Unorm
    );

    REQUIRE(nullptr != info.texture);
    REQUIRE(1 == loader.getUseCount(info.texture));
    loader.unloadTexture(info.texture);
    stbi_image_free(pixels);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("unique textures should be able to be added with correct use count after having been previously unloaded", "[texture][loading][asset system][add][remove][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    MTL::Texture* texture1 = loader.loadTexture(
        std::filesystem::path("assets/test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatBGRA8Unorm
    );

    REQUIRE(nullptr != texture1);
    REQUIRE(1 == loader.getUseCount(texture1));

    loader.unloadTexture(texture1);

    REQUIRE(0 == loader.getUseCount(texture1));

    MTL::Texture* texture2 = loader.loadTexture(
        std::filesystem::path("assets/test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatBGRA8Unorm
    );
    
    REQUIRE(nullptr != texture2);
    REQUIRE(1 == loader.getUseCount(texture2));

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("textures with only one channel should be able to be loaded", "[texture][loading][asset system][add][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader loader(metalDevice);

    MTL::Texture* texture = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormatR8Unorm,
        1
    );

    REQUIRE(nullptr != texture);
    REQUIRE(1 == loader.getUseCount(texture));

    metalDevice->release();
    autoReleasePool->release();
}