/**
 * @file render_pipeline_library.cpp
 * @brief
 */

#include "render_pipeline_library.hpp"

RenderPipelineLibrary::RenderPipelineLibrary(MTL::Device* metal_device, MTL4::Compiler* metal_compiler)
{
    device = metal_device;
    compiler = metal_compiler;
}

RenderPipelineLibrary::~RenderPipelineLibrary()
{

}

MTL::RenderPipelineState* RenderPipelineLibrary::get(RenderPipelineHandle pipeline) const
{

}

void RenderPipelineLibrary::build_formats(MTL::PixelFormat pixel_format)
{
    for (RenderPipelineHandle i = 0; i < static_cast<RenderPipelineBitmap>(RenderPipelineFlags::FlagCount); ++i)
    {
        MTL::RenderPipelineState* pipeline_state = compile_render_pipeline(pixel_format);
        pipeline_state_objects[i] = pipeline_state;
    }
}

MTL::RenderPipelineState* RenderPipelineLibrary::compile_render_pipeline(MTL::PixelFormat pixel_format)
{
    MTL4::RenderPipelineDescriptor* descriptor = configure_render_pipeline_descriptor(pixel_format);
    
    NS::Error* error = nullptr;
    
    MTL::RenderPipelineState* pipeline_state;
    pipeline_state = compiler->newRenderPipelineState(
        descriptor,
        nullptr,
        &error
    );

    descriptor->release();

    assert(nullptr != pipeline_state);
    return pipeline_state;
}

MTL4::RenderPipelineDescriptor* RenderPipelineLibrary::configure_render_pipeline_descriptor(MTL::PixelFormat pixel_format)
{
    MTL4::RenderPipelineDescriptor* render_pipeline_descriptor = MTL4::RenderPipelineDescriptor::alloc()->init();
    render_pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat(pixel_format);
    render_pipeline_descriptor->setVertexFunctionDescriptor(make_vertex_shader_configuration());
    render_pipeline_descriptor->setFragmentFunctionDescriptor(make_fragment_shader_configuration());
}

MTL4::LibraryFunctionDescriptor* RenderPipelineLibrary::make_vertex_shader_configuration()
{
    
}

MTL4::LibraryFunctionDescriptor* RenderPipelineLibrary::make_fragment_shader_configuration()
{

}