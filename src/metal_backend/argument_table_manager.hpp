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
};

enum class ShaderType : uint8_t
{
    MINIMUM_POSSIBLE_PBR_SHADER_TYPE_VALUE,
    PBRVertex = MINIMUM_POSSIBLE_PBR_SHADER_TYPE_VALUE,
    PBRFragment,
    MAXIMUM_POSSIBLE_PBR_SHADER_TYPE_VALUE,

    MINIMUM_POSSIBLE_TOON_SHADER_TYPE_VALUE,
    ToonVertex = MINIMUM_POSSIBLE_TOON_SHADER_TYPE_VALUE,
    ToonFragment,
    MAXIMUM_POSSIBLE_TOON_SHADER_TYPE_VALUE,

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

    void apply_tables(MTL4::RenderCommandEncoder* encoder);

private:

    constexpr MTL4::ArgumentTable* get_argument_table_from_shader_type(ShaderType shader_type) const;

    void create_vertex_argument_table(MaterialType material_type);
    void create_fragment_argument_table(MaterialType material_type);

    MTL4::ArgumentTable* vertex_argument_table;
    MTL4::ArgumentTable* fragment_argument_table;

    MTL::Device* device;
};