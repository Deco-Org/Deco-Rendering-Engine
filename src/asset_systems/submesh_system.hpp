/**
 * @file submesh_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "utils/AAPLMathUtilities.h"
#include "tools/synchronized_buffer.hpp"
#include "ufbx.h"

using SubmeshHandle = uint32_t;

enum class SubmeshSkinningProperty: char
{
    Unskinned = 0,
    Skinned,
};

struct SubmeshRenderThreadInputBufferEntry
{
    SubmeshHandle submesh;
    Vertex* vertices;
    size_t numberOfVertices;
};

struct SubmeshList
{
    SubmeshHandle* submesh;
    size_t n;
};

class SubmeshSystem
{
    public:

    /**
     * Queue the submeshes of a mesh to be added to the system
     */
    SubmeshHandle add(ufbx_mesh* mesh);

    /**
     * Remove a list of submeshes by handle
     * @param submeshes A `submesh_list`
     */
    void remove(SubmeshList submeshes);

    /**
     * Adds entries in `inputEntries` to the submesh system.
     * @warning Should only be called on the render thread.
     */
    void drainInputBuffer();

    /**
     * Drains consumed handles from the `outputHandles` buffer.
     * @note This is used to communicate with the render thread.
     */
    void drainOutputBuffer();

    std::vector<MetalBufferPtr> vertexBuffers;
    std::vector<MetalBufferPtr> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of submeshes
    std::vector<simd_float3> boundsMax; // Max bounds of submeshes
    std::vector<SubmeshSkinningProperty> skinningProperty;
    std::vector<uint32_t> boneCounts;

    private:
    std::vector<SubmeshHandle> freeHandles;
    SynchronizedBuffer<SubmeshRenderThreadInputBufferEntry> inputEntries;
    SynchronizedBuffer<SubmeshHandle> outputHandles;
};