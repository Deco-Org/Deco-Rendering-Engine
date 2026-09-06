// #include <metal_graphics.h>
#include <metal_stdlib>
using namespace metal;

// Function constants
constant bool is_skinned [[function_constant(0)]];
constant bool is_translucent [[function_constant(1)]];

struct VertexIn {
    float3 position         [[attribute(0)]];
    float3 normal           [[attribute(1)]];
    float2 tex_coord         [[attribute(2)]];

    // Only used when isSkinned is true
    float4 bone_weights      [[attribute(3)]];
    ushort4 bone_indices     [[attribute(4)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 world_position;
    float3 normal;
    float2 tex_coord;
};

struct TransformData {
    float4x4 view_matrix;
    float4x4 projection_matrix;
};

vertex VertexOut vertex_pbr_shader(
    VertexIn in [[stage_in]],
    constant float4x4* transforms [[buffer(0)]],
    constant float4x4* skinning_matrices [[buffer(1)]],
    constant TransformData& cameraData [[buffer(2)]],
    uint instance_id [[instance_id]])
{
    float4 position = float4(in.position, 1.0);

    if (is_skinned) {
        float4x4 skin_matrix = 
            in.bone_weights[0] * skinning_matrices[in.bone_indices[0]] +
            in.bone_weights[1] * skinning_matrices[in.bone_indices[1]] +
            in.bone_weights[2] * skinning_matrices[in.bone_indices[2]] +
            in.bone_weights[3] * skinning_matrices[in.bone_indices[3]];
        position = skin_matrix * position;
    }

    float4x4 world_matrix = transforms[instance_id];

    VertexOut out;
    out.world_position = (world_matrix * position).xyz;
    out.position = cameraData.projection_matrix * cameraData.view_matrix * float4(out.world_position, 1.0);
    out.normal = normalize((world_matrix * float4(in.normal, 0.0)).xyz);
    out.tex_coord = in.tex_coord;
    return out;
}

// Fragment Shader - Placeholder
struct PBRMaterial {
    float4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
    float ambient_occlusion_factor;
    float4 emission_color_and_factor;
};

fragment float4 fragment_pbr_shader(
    VertexOut in [[stage_in]],
    texture2d<float> albedo_texture     [[texture(0)]],
    texture2d<float> normal_texture     [[texture(1)]],
    texture2d<float> orm_texture        [[texture(2)]],
    texture2d<float> emission_texture   [[texture(3)]],
    sampler texture_sampler             [[sampler(0)]],
    constant PBRMaterial& material      [[buffer(0)]])
{
    float4 albedo = albedo_texture.sample(texture_sampler, in.tex_coord)* material.base_color_factor;

    if (is_translucent && albedo.a < 0.01) {
        discard_fragment();
    }

    float3 orm = orm_texture.sample(texture_sampler, in.tex_coord).rgb;
    float ambient_occlusion = orm.r;
    float roughness = orm.g * material.roughness_factor;
    float metallic = orm.b * material.metallic_factor;

    float4 emission = emission_texture.sample(texture_sampler, in.tex_coord);
    float3 emission_color = emission.rgb * emission.a * material.emission_color_and_factor.rgb * material.emission_color_and_factor.a;

    float3 color = albedo.rgb * ambient_occlusion * emission_color;

    return float4(color, is_translucent ? albedo.a : 1.0);
}