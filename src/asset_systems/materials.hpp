/**
 * @file materials.hpp
 * @brief Structures for materials
 */

using MaterialHandle = uint16_t;
inline constexpr MaterialHandle INVALID_MATERIAL = UINT16_MAX;

inline constexpr simd_float4 DEFAULT_COLOR_4_CHANNELS = { 1.0f, 1.0f, 1.0f, 1.0f };
inline constexpr simd_float3 DEFAULT_COLOR_3_CHANNELS = { 1.0f, 1.0f, 1.0f };

enum class MaterialType : uint8_t
{
    PBR = 0,
    Toon = 0,
    Unknown = (uint8_t)(-1)
};

struct PBRMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* normalTexture = nullptr;
    MTL::Texture* metallicRoughnessAoTexture = nullptr;
    MTL::Texture* emission = nullptr;

    simd_float4 baseColorFactor = DEFAULT_COLOR_4_CHANNELS;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float ambientOcclusionFactor = 1.0f;
    simd_float4 emissionColorAndFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct ToonMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* shadowThresholdTexture = nullptr;
    
    simd_float4 baseColorFactor = DEFAULT_COLOR_4_CHANNELS;
    float shadowThresholdValue = 0.5f;
    
    float shadowSoftness = 0.0f;
};

struct Material
{
    MaterialType type = MaterialType::Unknown;
    bool isTombstone = false;
    union
    {
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};
