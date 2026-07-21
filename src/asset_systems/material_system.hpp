/**
 * @file material_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "utils/AAPLMathUtilities.h"
#include "tools/synchronized_buffer.hpp"
#include "texture_loader.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"

using MaterialHandle = uint16_t;
using PBRMaterialHandle = uint16_t;
using ToonMaterialHandle = uint16_t;
inline constexpr MaterialHandle INVALID_MATERIAL = UINT16_MAX;

enum class MaterialType : uint8_t
{
    PBR = 0,
    Toon = 0,
};

struct PBRMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* normalTexture = nullptr;
    MTL::Texture* metallicRoughnessAoTexture = nullptr;
    MTL::Texture* emission = nullptr;
    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct ToonMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* shadowThresholdTexture = nullptr;
    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float shadowSoftness = 0.0f;
};

struct Material
{
    MaterialType type;
    bool isTombstone;
    union
    {
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};

struct MaterialEntry
{
    MaterialType type;
    union
    {
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};

DECO_ENGINE_LIST_TYPE(MaterialHandleList, MaterialHandle);
DECO_ENGINE_LIST_TYPE(MaterialEntryList, MaterialEntry);

class MaterialSystem
{
    public:
    MaterialSystem(TextureLoader* loader);

    MaterialHandleList add(MaterialEntryList materials);

    void remove(MaterialHandleList materials);

    void updateMaterial(MaterialHandle handle, Material& material);

    /**
     * 
     * @warning This should only be called on the render thread.
     */
    void drainAdditionsInputBuffer();

    /**
     * 
     * @note This is used to communicate with the render thread.
     */
    std::vector<MaterialHandle> getAndDrainAdditionsOutputBuffer();

    /**
     * 
     * @warning This should only be called on the render thread.
     */
    void drainRemovalsInputBuffer();

    /**
     * 
     * @note This is used to communicate with the render thread.
     */
    void drainRemovalsOutputBufferAndUnloadResources();

    std::vector<Material> materials;

    SystemInputBuffer<Material, MaterialHandle> additionsInputBuffer;
    SystemOutputBuffer<MaterialHandle> additionsOutputBuffer;
    SynchronizedBuffer<MaterialHandle> removalsInputBuffer;
    SynchronizedBuffer<Material> removalsOutputBuffer;
    
    private:
    TextureLoader* textureLoader;
    std::vector<MaterialHandle> freeMaterialHandles;
};