/**
 * @file material_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
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
    Unknown = (uint8_t)(-1)
};

struct PBRMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* normalTexture = nullptr;
    MTL::Texture* metallicRoughnessAoTexture = nullptr;
    MTL::Texture* emission = nullptr;

    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float ambientOcclusionFactor = 1.0f;
    simd_float3 emissionFactor = { 0.0f, 0.0f, 0.0f };
};

struct ToonMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* shadowThresholdTexture = nullptr;
    
    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
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

struct MaterialEntry
{
    MaterialType type;
    union
    {
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};

struct MaterialRenderThreadInputBufferEntry
{
    MaterialHandle handle = INVALID_MATERIAL;
    Material material = {};
};

DECO_ENGINE_LIST_TYPE(MaterialHandleList, MaterialHandle);
DECO_ENGINE_LIST_TYPE(MaterialEntryList, MaterialEntry);

class MaterialSystem
{
    public:
    MaterialSystem(TextureLoader* loader);

    std::vector<MaterialHandle> add(MaterialEntryList materials);

    std::vector<MaterialHandle> add(ufbx_material_list* materials);

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
    std::vector<MaterialHandle> getItemsAndDrainAdditionsOutputBuffer();

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

    SystemInputBuffer<MaterialRenderThreadInputBufferEntry, MaterialHandle> additionsInputBuffer;
    SystemOutputBuffer<MaterialHandle> additionsOutputBuffer;
    SynchronizedBuffer<MaterialHandle> removalsInputBuffer;
    SynchronizedBuffer<Material> removalsOutputBuffer;

    MaterialHandle largestHandle = INVALID_MATERIAL;
    
    private:
    Material* loadMaterial(ufbx_material* material);
    MaterialHandle* getNextNHandles(size_t n);
    void addInputEntriesToAdditionsBuffer(MaterialRenderThreadInputBufferEntry* entries, size_t count);

    TextureLoader* textureLoader;
    std::vector<MaterialHandle> freeHandles;
};