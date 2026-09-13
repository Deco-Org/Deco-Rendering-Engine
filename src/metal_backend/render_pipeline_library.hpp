/**
 * @file render_pipeline_library.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>

using RenderPipelineHandle = uint8_t;
using RenderPipelineBitmap = uint8_t;

enum class RenderPipelineFlags : RenderPipelineBitmap
{
    None = 0,

    // PBR = 0 << 0,
    Toon = 1 << 0,

    // Static = 0 << 1,
    Skinned = 1 << 1,

    // Opaque = 0 << 2,
    Translucent = 1 << 2,
    
    FlagCount = 3,
    PipelineCount = 1 << FlagCount
};

class RenderPipelineLibrary
{
    public:
    RenderPipelineLibrary(MTL::Device* metal_device, MTL4::Compiler* metal_compiler);
    ~RenderPipelineLibrary();

    MTL::RenderPipelineState* get(RenderPipelineHandle pipeline) const;

    /**
     * Builds render pipeline states
     */
    void build_states(MTL::PixelFormat pixel_format);

    /**
     * Destroys render pipeline states
     */
    void destroy_states();

    private:
    MTL::RenderPipelineState* compile_render_pipeline(
        MTL::PixelFormat pixel_format, 
        RenderPipelineBitmap pipeline_flags
    );

    MTL4::RenderPipelineDescriptor* configure_render_pipeline_descriptor(
        MTL::PixelFormat pixel_format,
        RenderPipelineBitmap pipeline_flags
    );

    MTL4::SpecializedFunctionDescriptor* make_vertex_shader_configuration(RenderPipelineBitmap pipeline_flags);
    MTL4::SpecializedFunctionDescriptor* make_fragment_shader_configuration(RenderPipelineBitmap pipeline_flags);

    MTL::VertexDescriptor* make_vertex_descriptor();

    MTL::RenderPipelineState* pipeline_state_objects[static_cast<RenderPipelineBitmap>(RenderPipelineFlags::PipelineCount)] = {};
    MTL4::Compiler* compiler = nullptr;
    MTL::Device* device = nullptr;
};

// Inlines

inline RenderPipelineFlags operator&(RenderPipelineFlags a, RenderPipelineFlags b) {
    return static_cast<RenderPipelineFlags>(
        static_cast<RenderPipelineBitmap>(a) & static_cast<RenderPipelineBitmap>(b)
    );
}