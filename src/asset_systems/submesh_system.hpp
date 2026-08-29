/**
 * @file submesh_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "tools/synchronized_buffer.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"

using SubmeshHandle = uint32_t;
static constexpr SubmeshHandle INVALID_SUBMESH_HANDLE = UINT32_MAX;
static constexpr NS::UInteger INVALID_INDEX_COUNT = NS::UIntegerMax;

DECO_ENGINE_LIST_TYPE(SubmeshList, SubmeshHandle);

enum class SubmeshSkinningProperty: char
{
    Unskinned = 0,
    Skinned,
};

struct SubmeshRenderThreadInputBufferEntry
{
    SubmeshHandle handle;
    MTL::Buffer* vertexBuffer;
    MTL::Buffer* indexBuffer;
    NS::UInteger indexCount;
    simd_float3 boundsMin;
    simd_float3 boundsMax;
    SubmeshSkinningProperty skinningProperty;
    uint32_t boneCount;
};

class SubmeshSystem
{
    public:

    SubmeshSystem(MTL::Device* metalDevice = nullptr);

    ~SubmeshSystem();

    /**
     * Queue the submeshes of a given mesh to be added to the system.
     * @param mesh The mesh that holds the submeshes that are to be added.
     * @returns The handles of the queued submeshes.
     */
    std::vector<SubmeshHandle> add(ufbx_mesh* mesh);
    
    /**
     * Queue given submeshes of a mesh to be added to the system
     * @param mesh The parent mesh of the submeshes
     * @param submeshes The submeshes that are to be added to the system.
     */
    SubmeshHandle add(
        ufbx_mesh* mesh, 
        ufbx_mesh_part* submesh);

    /**
     * Remove a list of submeshes by handle
     * @param submeshes A list of submeshes to be removed
     */
    void remove(SubmeshList submeshes);

    /**
     * Adds entries in `inputEntries` to the submesh system and drains `inputEntries`.
     * @warning Should only be called on the render thread.
     */
    void drainAdditionsInputBuffer();

    /**
     * Drains the removal buffer and removes items from the system.
     * @warning Should only be called on the render thread.
     */
    void drainRemovalBuffer();

    /**
     * Drains consumed handles from the `outputHandles` buffer.
     * @note This is used to communicate with the render thread.
     */
    std::vector<SubmeshHandle> getItemsAndDrainOutputBuffer();

    /**
     * @returns true if the submesh is a tombstone (the handle is free)
     * @warning This should only be called on the render thread.
     */
    bool isTombstone(SubmeshHandle handle) const;

    std::vector<MTL::Buffer*> vertexBuffers;
    std::vector<MTL::Buffer*> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of submeshes
    std::vector<simd_float3> boundsMax; // Max bounds of submeshes
    std::vector<SubmeshSkinningProperty> skinningProperties;
    std::vector<uint32_t> boneCounts;
    std::vector<SubmeshHandle> freeHandles;

    SubmeshHandle largestHandle = INVALID_SUBMESH_HANDLE;
    
    private:
    SubmeshRenderThreadInputBufferEntry generateInputEntryForSubmesh(
        ufbx_mesh* parent,
        ufbx_mesh_part* submesh,
        SubmeshHandle handle);

    void addInputEntriesToAdditionsBuffer(
        SubmeshRenderThreadInputBufferEntry* entries,
        size_t count
    );
        
    void createAndFillVertexAndIndexBuffers(
        SubmeshRenderThreadInputBufferEntry& entry,
        std::vector<Vertex>& vertices, 
        std::vector<uint32_t>& indices);
            
    SubmeshHandle* getNextNHandles(size_t n);
    
    SystemInputBuffer<SubmeshRenderThreadInputBufferEntry, SubmeshHandle> inputEntries;
    SynchronizedBuffer<SubmeshHandle> removalBuffer;
    SystemOutputBuffer<SubmeshHandle> outputHandles;
    MTL::Device* device;
};

inline bool SubmeshSystem::isTombstone(SubmeshHandle handle) const { return indexCounts[handle] == INVALID_INDEX_COUNT; }