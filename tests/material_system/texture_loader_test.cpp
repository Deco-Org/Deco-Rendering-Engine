
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "asset_systems/texture_loader.hpp"
#include "texture_loader_test_fixture.hpp"
#include "test_utils.hpp"
#include <stb_image.h>

TEST_CASE("attempting adding a texture that does not exist should return nullptr", "[texture][loading][asset system][add][error handling][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader loader(metalDevice);

    MTL::Texture* texture = loader.loadTexture(
        std::filesystem::path("assets/nonexistent_texture.png")
    ).texture;

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
        ).texture;
        REQUIRE(nullptr != texture);
        REQUIRE(1 == loader.getUseCount(texture));
    }

    SECTION("adding a previously loaded texture should increment the use count of the texture by 1")
    {
        MTL::Texture* texture1 = loader.loadTexture(
            std::filesystem::path("assets/test_cube_texture.png"),
            MTL::PixelFormat::PixelFormatBGRA8Unorm
        ).texture;

        REQUIRE(nullptr != texture1);
        REQUIRE(1 == loader.getUseCount(texture1));

        MTL::Texture* texture2 = loader.loadTexture(
            std::filesystem::path("assets/test_cube_texture.png"),
            MTL::PixelFormat::PixelFormatBGRA8Unorm
        ).texture;

        REQUIRE(nullptr != texture2);
        REQUIRE(2 == loader.getUseCount(texture2));
        REQUIRE(2 == loader.getUseCount(texture1));
        REQUIRE(texture1 == texture2);
    }
    
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a texture and marking it as being used by the render thread should designate the texture as being used by the render thread", "[texture][loading][asset system][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    MTL::Texture* texture = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    ).texture;
    REQUIRE(nullptr != texture);

    REQUIRE(false == loader.textureIsOnRenderThread(texture));
    loader.markTextureAsUsedByRenderThread(texture);
    REQUIRE(true == loader.textureIsOnRenderThread(texture));

    // texture->release();
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a packed texture with pre-existing components should increment the use counts of its components", "[texture][loading][asset system][add][texture packing][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);
    TextureLoader::AddedTextureInfo texture0Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture0 = texture0Info.texture;
    TextureLoader::AddedTextureInfo texture1Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture1 = texture1Info.texture;
    TextureLoader::AddedTextureInfo texture2Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture2 = texture2Info.texture;
    TextureLoader::AddedTextureInfo texture3Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture3 = texture3Info.texture;
    REQUIRE(nullptr != texture0);
    REQUIRE(nullptr != texture1);
    REQUIRE(nullptr != texture2);
    REQUIRE(nullptr != texture3);

    MTL::Texture* packedTexture = loader.loadPackedTexture(
        texture0Info.handle,
        texture1Info.handle,
        texture2Info.handle,
        texture3Info.handle
    ).texture;
    REQUIRE(nullptr != packedTexture);

    // The packed texture should have the same dimensions as texture 0.
    CAPTURE(texture0, packedTexture);
    REQUIRE(texture0->width() == packedTexture->width());

    // The packed texture should be comprised of data from all three textures
    packedTextureShouldBeComprisedOfDataFromSourceTextures(
        packedTexture,
        texture0,
        texture1,
        texture2,
        texture3
    );
    
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a packed texture with pre-existing components that have been previously loaded more than once should not access the previously loaded textures", "[texture][loading][asset system][add][texture packing][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    TextureLoader::AddedTextureInfo texture0Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture0 = texture0Info.texture;

    TextureLoader::AddedTextureInfo texture1Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture1 = texture1Info.texture;

    TextureLoader::AddedTextureInfo texture2Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture2 = texture2Info.texture;

    TextureLoader::AddedTextureInfo texture3Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture3 = texture3Info.texture;

    REQUIRE(nullptr != texture0);
    REQUIRE(nullptr != texture1);
    REQUIRE(nullptr != texture2);
    REQUIRE(nullptr != texture3);

    // Temporarily 

    MTL::Texture* packedTexture = loader.loadPackedTexture(
        texture0Info.handle,
        texture1Info.handle,
        texture2Info.handle,
        texture3Info.handle
    ).texture;
    REQUIRE(nullptr != packedTexture);

    // The packed texture should have the same dimensions as texture 0.
    CAPTURE(texture0, packedTexture);
    REQUIRE(texture0->width() == packedTexture->width());

    // The packed texture should be comprised of data from all three textures
    packedTextureShouldBeComprisedOfDataFromSourceTextures(
        packedTexture,
        texture0,
        texture1,
        texture2,
        texture3
    );
    
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a packed texture with only some pre-existing components should increment the use counts of its components", "[texture][loading][asset system][add][texture packing][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    TextureLoader::AddedTextureInfo texture0Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture0 = texture0Info.texture;

    TextureLoader::AddedTextureInfo texture1Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture1 = texture1Info.texture;

    TextureLoader::AddedTextureInfo texture2Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture2 = texture2Info.texture;

    REQUIRE(nullptr != texture0);
    REQUIRE(nullptr != texture1);
    REQUIRE(nullptr != texture2);

    uint32_t previousNumberOfReferencesForTextures[] = {
        loader.getUseCount(texture0Info.handle),
        loader.getUseCount(texture1Info.handle),
        loader.getUseCount(texture2Info.handle),
    };

    MTL::Texture* packedTexture = loader.loadPackedTexture(
        texture0Info.handle,
        texture1Info.handle,
        texture2Info.handle
    ).texture;
    REQUIRE(nullptr != packedTexture);

    // The packed texture should have the same dimensions as texture 0.
    CAPTURE(texture0, texture1, texture2, packedTexture);
    REQUIRE(texture0->width() == packedTexture->width());

    packedTextureShouldBeComprisedOfDataFromSourceTextures(
        packedTexture,
        texture0,
        texture1,
        texture2
    );

    // The use counts should be incremented
    REQUIRE(previousNumberOfReferencesForTextures[0] + 2 == loader.getUseCount(texture0Info.handle));
    REQUIRE(previousNumberOfReferencesForTextures[1] + 1 == loader.getUseCount(texture1Info.handle));
    REQUIRE(previousNumberOfReferencesForTextures[2] + 2 == loader.getUseCount(texture2Info.handle));
    
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
    ).texture;

    REQUIRE(nullptr != texture);
    REQUIRE(1 == loader.getUseCount(texture));

    loader.unloadTexture(texture);

    REQUIRE(0 == loader.getUseCount(texture));
    sizeof(TextureLoader::TrackedTexture);

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
    ).texture;

    REQUIRE(nullptr != texture1);
    REQUIRE(1 == loader.getUseCount(texture1));

    loader.unloadTexture(texture1);

    REQUIRE(0 == loader.getUseCount(texture1));

    MTL::Texture* texture2 = loader.loadTexture(
        std::filesystem::path("assets/test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatBGRA8Unorm
    ).texture;
    
    REQUIRE(nullptr != texture2);
    REQUIRE(1 == loader.getUseCount(texture2));

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("removing a packed texture should decrease the use count of its components", "[texture][loading][asset system][remove][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();
    TextureLoader loader(metalDevice);

    TextureLoader::AddedTextureInfo texture0Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture0 = texture0Info.texture;

    TextureLoader::AddedTextureInfo texture1Info = loader.loadTexture(
        std::filesystem::path("assets/test_cube_one_channel_texture_02.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture1 = texture1Info.texture;

    TextureLoader::AddedTextureInfo texture2Info = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormat::PixelFormatR8Unorm
    );
    MTL::Texture* texture2 = texture2Info.texture;

    REQUIRE(nullptr != texture0);
    REQUIRE(nullptr != texture1);
    REQUIRE(nullptr != texture2);

    
    MTL::Texture* packedTexture = loader.loadPackedTexture(
        texture0Info.handle,
        texture1Info.handle,
        texture2Info.handle
    ).texture;
    REQUIRE(nullptr != packedTexture);
    
    // The packed texture should have the same dimensions as texture 0.
    CAPTURE(texture0, texture1, texture2, packedTexture);
    REQUIRE(texture0->width() == packedTexture->width());
    
    packedTextureShouldBeComprisedOfDataFromSourceTextures(
        packedTexture,
        texture0,
        texture1,
        texture2
    );
    
    uint32_t previousNumberOfReferencesForTextures[] = {
        loader.getUseCount(texture0Info.handle),
        loader.getUseCount(texture1Info.handle),
        loader.getUseCount(texture2Info.handle),
    };

    loader.unloadTexture(packedTexture);

    // The use counts should be decremented
    REQUIRE(previousNumberOfReferencesForTextures[0] - 2 == loader.getUseCount(texture0Info.handle));
    REQUIRE(previousNumberOfReferencesForTextures[1] - 1 == loader.getUseCount(texture1Info.handle));
    REQUIRE(previousNumberOfReferencesForTextures[2] - 2 == loader.getUseCount(texture2Info.handle));
    
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
    ).texture;

    REQUIRE(nullptr != texture);
    REQUIRE(1 == loader.getUseCount(texture));

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("textures should be able to be accessed through their handles", "[texture][loading][asset system][access][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader loader(metalDevice);

    TextureLoader::AddedTextureInfo textureInfo = loader.loadTexture(
        std::filesystem::path("assets/single_channel_test_cube_texture.png"),
        MTL::PixelFormatR8Unorm,
        1
    );

    REQUIRE(textureInfo.texture == loader.getTexture(textureInfo.handle));

    metalDevice->release();
    autoReleasePool->release();
}