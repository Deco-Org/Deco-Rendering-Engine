/**
 * @file material_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "tools/synchronized_buffer.hpp"
#include "texture_loader.hpp"
#include "materials.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"

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

    std::vector<MaterialHandle> add(ufbx_material_list* materials, MaterialType type = MaterialType::PBR);

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
    
    std::vector<MaterialHandle> freeHandles;
    MaterialHandle largestHandle = INVALID_MATERIAL;
    
    private:
    Material* loadMaterial(ufbx_material* material, MaterialType type);
    float getScalarValueFromUfbxMaterialMap(ufbx_material_map& materialMap);
    simd_float3 getThreeChannelColorFromUfbxMaterialMap(ufbx_material_map& materialMap);
    simd_float4 getFourChannelColorFromUfbxMaterialMap(ufbx_material_map& materialMap);
    MaterialHandle* getNextNHandles(size_t n);
    void addInputEntriesToAdditionsBuffer(MaterialRenderThreadInputBufferEntry* entries, size_t count);

    TextureLoader* textureLoader;
};