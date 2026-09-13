#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/argument_table_manager.hpp"

TEST_CASE("binding resources should not cause validation errors", "[argument table][metal]")
{
    // This test is basically just a giant ("things shouldn't crash" test)
    
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandBuffer* command_buffer = device->newCommandBuffer();

    MTL::TextureDescriptor* msaa_texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    msaa_texture_descriptor->setTextureType(MTL::TextureType2DMultisample);
    msaa_texture_descriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaa_texture_descriptor->setWidth(1024);
    msaa_texture_descriptor->setHeight(1024);
    msaa_texture_descriptor->setSampleCount(4);
    msaa_texture_descriptor->setUsage(MTL::TextureUsageRenderTarget);

    MTL::Texture* msaa_render_target_texture = device->newTexture(msaa_texture_descriptor);

    MTL::TextureDescriptor* depth_texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    depth_texture_descriptor->setTextureType(MTL::TextureType2DMultisample);
    depth_texture_descriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depth_texture_descriptor->setWidth(1024);
    depth_texture_descriptor->setHeight(1024);
    depth_texture_descriptor->setUsage(MTL::TextureUsageRenderTarget);
    depth_texture_descriptor->setSampleCount(4);
    depth_texture_descriptor->setStorageMode(MTL::StorageModePrivate);

    MTL::Texture* depth_texture = device->newTexture(depth_texture_descriptor);

    msaa_texture_descriptor->release();
    depth_texture_descriptor->release();

    MTL4::RenderPassDescriptor* render_pass_descriptor = MTL4::RenderPassDescriptor::alloc()->init();

    INFO("Setting render target width");
    render_pass_descriptor->setRenderTargetWidth(1024);
    render_pass_descriptor->setRenderTargetHeight(1024);
    render_pass_descriptor->setDefaultRasterSampleCount(4);

    MTL::RenderPassColorAttachmentDescriptor* color_attachment = render_pass_descriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = render_pass_descriptor->depthAttachment();

    color_attachment->setTexture(msaa_render_target_texture);
    color_attachment->setLoadAction(MTL::LoadActionClear);
    color_attachment->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    color_attachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(depth_texture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);

    MTL::TextureDescriptor* resolve_texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    resolve_texture_descriptor->setTextureType(MTL::TextureType2D);
    resolve_texture_descriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    resolve_texture_descriptor->setWidth(1024);
    resolve_texture_descriptor->setHeight(1024);
    resolve_texture_descriptor->setUsage(MTL::TextureUsageRenderTarget);
    MTL::Texture* resolve_texture = device->newTexture(resolve_texture_descriptor);
    resolve_texture_descriptor->release();

    color_attachment->setResolveTexture(resolve_texture);

    MTL4::CommandAllocator* command_allocator = device->newCommandAllocator();
    command_buffer->beginCommandBuffer(command_allocator);

    MTL4::RenderCommandEncoder* render_command_encoder = command_buffer->renderCommandEncoder(render_pass_descriptor); // Crash happens here
    render_pass_descriptor->release();

    ArgumentTableManager* argument_table_manager = new ArgumentTableManager(device);

    MTL::Buffer* some_buffer = device->newBuffer(512, MTL::ResourceStorageModeShared);
    MTL::Buffer* some_other_buffer = device->newBuffer(512, MTL::ResourceStorageModeShared);
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


    SECTION("buffers should be able to be successfully bound")
    {
        argument_table_manager->bind_buffer(
            some_buffer, 
            static_cast<RenderingArgumentSlot>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::Transforms),
            ShaderType::PBRVertex
        );

        argument_table_manager->bind_buffer(
            some_other_buffer,
            static_cast<RenderingArgumentSlot>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::Material),
            ShaderType::PBRFragment
        );
    }

    SECTION("textures should be able to be successfully bound")
    {
        argument_table_manager->bind_texture(
            some_texture,
            static_cast<RenderingArgumentSlot>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::AlbedoTexture),
            ShaderType::PBRFragment
        );
    }

    SECTION("samplers should be able to be successfully bound")
    {
        argument_table_manager->bind_sampler(
            some_sampler_state,
            static_cast<RenderingArgumentSlot>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::Sampler),
            ShaderType::PBRFragment
        );
    }

    argument_table_manager->apply_tables(render_command_encoder);

    delete argument_table_manager;
    if (some_buffer)
        some_buffer->release();
    if (some_other_buffer)
        some_other_buffer->release();
    if (some_texture)
        some_texture->release();
    if (some_sampler_state)
        some_sampler_state->release();
    device->release();
    autorelease_pool->release();
}