/**
 * @file argument_table_manager.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "algorithm"
#include "asset_systems/materials.hpp"

using RenderingArgumentSlot = uint8_t;

namespace ArgumentSlots
{
    enum class PBRRenderingArgumentSlot : RenderingArgumentSlot
    {
        Transforms = 0,
        SkinningMatrices = 1,
        CameraData = 2,
        NUMBER_OF_PBR_VERTEX_BUFFERS,

        AlbedoTexture = 0,
        NormalTexture = 1,
        OrmTexture = 2,
        EmissionTexture = 3,
        NUMBER_OF_PBR_FRAGMENT_TEXTURES,

        Sampler = 0,
        NUMBER_OF_PBR_FRAGMENT_SAMPLERS,

        Material = 0,
        NUMBER_OF_PBR_FRAGMENT_BUFFERS,

        Invalid = static_cast<RenderingArgumentSlot>(-1)
    };

    enum class ToonRenderingArgumentSlot : RenderingArgumentSlot
    {
        Transforms = 0,
        SkinningMatrices = 1,
        CameraData = 2,
        NUMBER_OF_TOON_VERTEX_BUFFERS,

        AlbedoTexture = 0,
        ShadowThresholdTexture = 1,
        NUMBER_OF_TOON_FRAGMENT_TEXTURES,

        Sampler = 0,
        NUMBER_OF_TOON_FRAGMENT_SAMPLERS,

        Material = 0,
        NUMBER_OF_TOON_FRAGMENT_BUFFERS,

        Invalid = static_cast<RenderingArgumentSlot>(-1)
    };
};

enum class ShaderType : uint8_t
{
    VertexShader = 1 << 7,
    // FragmentShader is 0 << 7
    ComputeShader = 1 << 6,

    PBR = 0,
    Toon,

    PBRVertex = VertexShader | PBR,
    ToonVertex = VertexShader | Toon,

    PBRFragment = PBR,
    ToonFragment = Toon,

    Invalid = static_cast<uint8_t>(-1),
};

class ArgumentTableManager
{
public:
    ArgumentTableManager(MTL::Device* metal_device);
    ~ArgumentTableManager();

    void bind_buffer(MTL::Buffer* buffer, RenderingArgumentSlot argument_slot, ShaderType shader_type);
    void bind_texture(MTL::Texture* texture, RenderingArgumentSlot argument_slot, ShaderType shader_type);
    void bind_sampler(MTL::SamplerState* sampler_state, RenderingArgumentSlot argument_slot, ShaderType shader_type);

    void apply_tables(MTL4::RenderCommandEncoder* encoder, MaterialType material_type);

private:

    consteval ShaderType get_shader_type_from_material_type(MaterialType material_type) const;
    constexpr MTL4::ArgumentTable* get_argument_table_from_shader_type(ShaderType shader_type) const;

    void create_argument_table(ShaderType shader_type);

    MTL4::ArgumentTable* vertex_argument_table = nullptr;
    MTL4::ArgumentTable* pbr_fragment_argument_table = nullptr;
    MTL4::ArgumentTable* toon_fragment_argument_table = nullptr;

    MTL::Device* device = nullptr;
};


inline uint8_t operator&(ShaderType a, ShaderType b) {
    return static_cast<uint8_t>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}

inline uint8_t operator|(ShaderType a, ShaderType b) {
    return static_cast<uint8_t>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}