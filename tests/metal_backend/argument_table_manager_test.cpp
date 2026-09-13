#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/argument_table_manager.hpp"

TEST_CASE("binding resources should not cause validation errors", "[argument table][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();

    ArgumentTableManager argument_table_manager(device);

    MTL::Buffer* some_buffer = device->newBuffer(512, MTL::ResourceStorageModeShared);
    // MTL::Texture* some_texture = device->newTexture()
    MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    texture_descriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    texture_descriptor->setTextureType(MTL::TextureType2D);
    texture_descriptor->setWidth(128);
    texture_descriptor->setHeight(128);
    texture_descriptor->setStorageMode(MTL::StorageModeShared);
    texture_descriptor->setUsage(MTL::TextureUsageShaderRead);
    MTL::Texture* some_texture = device->newTexture(texture_descriptor);
    texture_descriptor->release();

    MTL::SamplerDescriptor* sampler_descriptor = MTL::SamplerDescriptor::alloc()->init();
    sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
    sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
    MTL::SamplerState* some_sampler_state = device->newSamplerState(sampler_descriptor);
    sampler_descriptor->release();


    SECTION("Buffers should be able to be successfully bound")
    {
        argument_table_manager.bind_buffer(
            some_buffer, 
            static_cast<RenderingArgumentSlot>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::Transforms),
            ShaderType::PBRVertex
        );
    }

    // argument_table_manager.

    device->release();
    autorelease_pool->release();
}