/**
 * @file materials.hpp
 * @brief Structures for materials
 */

#include <algorithm>

using MaterialHandle = uint16_t;
inline constexpr MaterialHandle INVALID_MATERIAL = UINT16_MAX;

inline constexpr simd_float4 DEFAULT_COLOR_4_CHANNELS = { 1.0f, 1.0f, 1.0f, 1.0f };
inline constexpr simd_float3 DEFAULT_COLOR_3_CHANNELS = { 1.0f, 1.0f, 1.0f };

enum class MaterialType : uint8_t
{
    PBR = 0,
    Toon = 1,
    Unknown = static_cast<std::underlying_type_t<MaterialType>>(-1),
};

enum class MaterialOpticalProperties : uint8_t
{
    Opaque = 0 << 1,
    Transparent = 1 << 1,
    
    Unknown = static_cast<std::underlying_type_t<MaterialOpticalProperties>>(-1),
};

enum class MaterialProperties : uint8_t
{
    PBR = 0 << 0,
    Toon = 1 << 0,

    Opaque = static_cast<std::underlying_type_t<MaterialProperties>>(MaterialOpticalProperties::Opaque),
    Transparent = static_cast<std::underlying_type_t<MaterialProperties>>(MaterialOpticalProperties::Transparent),
    UnknownOpticalProperties = 1 << 2,

    Unknown = static_cast<std::underlying_type_t<MaterialProperties>>(-1),
};

constexpr MaterialProperties operator&(MaterialProperties a, MaterialProperties b)
{
    using underlying = std::underlying_type_t<MaterialProperties>;
    return static_cast<MaterialProperties>(static_cast<underlying>(a) & static_cast<underlying>(b));
}

constexpr MaterialProperties operator|(MaterialProperties a, MaterialProperties b)
{
    using underlying = std::underlying_type_t<MaterialProperties>;
    return static_cast<MaterialProperties>(static_cast<underlying>(a) | static_cast<underlying>(b));
}

constexpr MaterialProperties operator|=(MaterialProperties& left, MaterialProperties right)
{
    left = left | right;
    return left;
}

enum class ORMChannel: uint8_t
{
    AmbientOcclusion = 0,
    Roughness = 1,
    Metallic = 2,
    Invalid = (uint8_t)(-1)
};

#define ufbxMaterialFeature(material, feature) material->features.features[offsetof(ufbx_material_features, feature)]

struct PBRMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* normalTexture = nullptr;
    MTL::Texture* AoRoughnessMetallicTexture = nullptr;
    MTL::Texture* emissionTexture = nullptr;

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

struct AnonymousMaterial {
    MTL::Texture* albedoTexture = nullptr;
};

struct Material
{
    MaterialType type = MaterialType::Unknown;
    bool isTombstone = false;
    union
    {
        AnonymousMaterial material;
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};

namespace MaterialTextureOffset
{
    using TextureOffset = uint8_t;

    enum class PBRTextureOffset: TextureOffset
    {
        Albedo = offsetof(Material, pbrMaterial.albedoTexture),
        Normal = offsetof(Material, pbrMaterial.normalTexture),
        ORM = offsetof(Material, pbrMaterial.AoRoughnessMetallicTexture),
        Emission = offsetof(Material, pbrMaterial.emissionTexture),
        // Combined Textures
        // ORM (252 - 255)
        AmbientOcclusion = (TextureOffset)(-4),
        Roughness,
        Metallic,
    };

    enum class ToonTextureOffset: TextureOffset
    {
        Albedo = offsetof(Material, toonMaterial.albedoTexture),
        Shadow = offsetof(Material, toonMaterial.shadowThresholdTexture),
    };

    enum class TextureCounts: TextureOffset
    {
        NUMBER_OF_PBR_TEXTURES = offsetof(Material, pbrMaterial.baseColorFactor) / sizeof(MTL::Texture) - (sizeof(Material::pbrMaterial.baseColorFactor) / 8),
        NUMBER_OF_TOON_TEXTURES = offsetof(Material, toonMaterial.baseColorFactor) / sizeof(MTL::Texture) - (sizeof(Material::toonMaterial.baseColorFactor) / 8),
        MAX_NUMBER_OF_TEXTURES_IN_MATERIAL = std::max(NUMBER_OF_PBR_TEXTURES, NUMBER_OF_TOON_TEXTURES),
    };
};