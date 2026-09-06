/**
 * @file render_pipeline_library.cpp
 * @brief
 */

#include "render_pipeline_library.hpp"
#include "default_metallib.h"
#include "vertex.hpp"
#include <dispatch/dispatch.h>

static const NS::String* vertex_pbr_shader_name = MTLSTR("vertex_pbr_shader");
static const NS::String* fragment_pbr_shader_name = MTLSTR("fragment_pbr_shader");

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
    for (RenderPipelineHandle i = 0; i < static_cast<RenderPipelineBitmap>(RenderPipelineFlags::PipelineCount); ++i)
    {
        MTL::RenderPipelineState* pipeline_state = compile_render_pipeline(pixel_format, i);
        pipeline_state_objects[i] = pipeline_state;
    }
}

MTL::RenderPipelineState* RenderPipelineLibrary::compile_render_pipeline(MTL::PixelFormat pixel_format, RenderPipelineBitmap pipeline_flags)
{
    MTL4::RenderPipelineDescriptor* descriptor = configure_render_pipeline_descriptor(pixel_format, pipeline_flags);
    
    NS::Error* error = nullptr;
    
    MTL::RenderPipelineState* pipeline_state;
    pipeline_state = compiler->newRenderPipelineState(
        descriptor,
        nullptr,
        &error
    );

    descriptor->release();

    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }
    
    assert(nullptr != pipeline_state);
    return pipeline_state;
}

MTL4::RenderPipelineDescriptor* RenderPipelineLibrary::configure_render_pipeline_descriptor(MTL::PixelFormat pixel_format, RenderPipelineBitmap pipeline_flags)
{
    MTL4::RenderPipelineDescriptor* render_pipeline_descriptor = MTL4::RenderPipelineDescriptor::alloc()->init();
    render_pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat(pixel_format);
    MTL4::SpecializedFunctionDescriptor* vertex_shader_config = make_vertex_shader_configuration(pipeline_flags);

    render_pipeline_descriptor->setVertexFunctionDescriptor(vertex_shader_config);
    render_pipeline_descriptor->setFragmentFunctionDescriptor(make_fragment_shader_configuration(pipeline_flags));

    render_pipeline_descriptor->setVertexDescriptor(make_vertex_descriptor());

    return render_pipeline_descriptor;
}

MTL4::SpecializedFunctionDescriptor* RenderPipelineLibrary::make_vertex_shader_configuration(RenderPipelineBitmap pipeline_flags)
{
    bool is_skinned = (static_cast<RenderPipelineFlags>(pipeline_flags) & RenderPipelineFlags::Skinned) != RenderPipelineFlags::None;
    bool is_translucent = (static_cast<RenderPipelineFlags>(pipeline_flags) & RenderPipelineFlags::Translucent) != RenderPipelineFlags::None;

    MTL4::LibraryFunctionDescriptor* base_function_descriptor = MTL4::LibraryFunctionDescriptor::alloc()->init();
    // MTL::Library* library = device->newLibrary(NS::String::string("default.metallib", NS::UTF8StringEncoding), nullptr);
    MTL::Library* library = load_shader_library();

    base_function_descriptor->setLibrary(library);
    base_function_descriptor->setName(vertex_pbr_shader_name);

    MTL::FunctionConstantValues* constant_values = MTL::FunctionConstantValues::alloc()->init();
    constant_values->setConstantValue(&is_skinned,      MTL::DataTypeBool,  NS::UInteger(0)); // Index 0
    constant_values->setConstantValue(&is_translucent,  MTL::DataTypeBool,  NS::UInteger(1)); // Index 1

    MTL4::SpecializedFunctionDescriptor* specialized_function_descriptor = MTL4::SpecializedFunctionDescriptor::alloc()->init();
    specialized_function_descriptor->setFunctionDescriptor(base_function_descriptor);
    specialized_function_descriptor->setConstantValues(constant_values);

    base_function_descriptor->release();
    constant_values->release();

    return specialized_function_descriptor;
}

MTL4::SpecializedFunctionDescriptor* RenderPipelineLibrary::make_fragment_shader_configuration(RenderPipelineBitmap pipeline_flags)
{
    bool is_skinned = (static_cast<RenderPipelineFlags>(pipeline_flags) & RenderPipelineFlags::Skinned) != RenderPipelineFlags::None;
    bool is_translucent = (static_cast<RenderPipelineFlags>(pipeline_flags) & RenderPipelineFlags::Translucent) != RenderPipelineFlags::None;

    MTL4::LibraryFunctionDescriptor* base_function_descriptor = MTL4::LibraryFunctionDescriptor::alloc()->init();
    base_function_descriptor->setLibrary(load_shader_library());
    base_function_descriptor->setName(fragment_pbr_shader_name);

    MTL::FunctionConstantValues* constant_values = MTL::FunctionConstantValues::alloc()->init();
    constant_values->setConstantValue(&is_skinned,      MTL::DataTypeBool, NS::UInteger(0)); // Index 0
    constant_values->setConstantValue(&is_translucent,  MTL::DataTypeBool, NS::UInteger(1)); // Index 1

    MTL4::SpecializedFunctionDescriptor* specialized_function_descriptor = MTL4::SpecializedFunctionDescriptor::alloc()->init();
    specialized_function_descriptor->setFunctionDescriptor(base_function_descriptor);
    specialized_function_descriptor->setConstantValues(constant_values);

    base_function_descriptor->release();
    constant_values->release();

    return specialized_function_descriptor;
}

MTL::VertexDescriptor* RenderPipelineLibrary::make_vertex_descriptor()
{
    MTL::VertexDescriptor* vertex_descriptor = MTL::VertexDescriptor::alloc()->init();

    vertex_descriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vertex_descriptor->attributes()->object(0)->setOffset(offsetof(Vertex, position));
    vertex_descriptor->attributes()->object(0)->setBufferIndex(0);

    vertex_descriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    vertex_descriptor->attributes()->object(1)->setOffset(offsetof(Vertex, normal));
    vertex_descriptor->attributes()->object(1)->setBufferIndex(0);

    vertex_descriptor->attributes()->object(2)->setFormat(MTL::VertexFormatFloat2);
    vertex_descriptor->attributes()->object(2)->setOffset(offsetof(Vertex, uv));
    vertex_descriptor->attributes()->object(2)->setBufferIndex(0);

    vertex_descriptor->attributes()->object(3)->setFormat(MTL::VertexFormatFloat4);
    vertex_descriptor->attributes()->object(3)->setOffset(offsetof(Vertex, bone_weights));
    vertex_descriptor->attributes()->object(3)->setBufferIndex(0);

    vertex_descriptor->attributes()->object(4)->setFormat(MTL::VertexFormatUShort4);
    vertex_descriptor->attributes()->object(4)->setOffset(offsetof(Vertex, bone_indices));
    vertex_descriptor->attributes()->object(4)->setBufferIndex(0);

    vertex_descriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    return vertex_descriptor;
}

MTL::Library* RenderPipelineLibrary::load_shader_library()
{
    dispatch_data_t data = dispatch_data_create(
        default_metallib,
        default_metallib_len,
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), 
        DISPATCH_DATA_DESTRUCTOR_DEFAULT
    );
    MTL::Library* library = device->newLibrary(data, nullptr);

    dispatch_release(data);

    assert(nullptr != library);

    return library;
}