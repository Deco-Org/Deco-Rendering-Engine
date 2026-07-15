/**
 * @file submesh_system.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "utils/AAPLMathUtilities.h"
#include "ufbx.h"

using SubmeshHandle = uint32_t;

class SubmeshSystem
{
    public:
    SubmeshHandle add(ufbx_mesh_part* part);
    void remove(SubmeshHandle submesh);

    std::vector<MetalBufferPtr> vertexBuffers;
    std::vector<MetalBufferPtr> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of submeshes
    std::vector<simd_float3> boundsMax; // Max bounds of submeshes
    std::vector<char> isSkinned;
    std::vector<uint32_t> boneCounts;

    private:
    std::vector<SubmeshHandle> freeHandles;
};