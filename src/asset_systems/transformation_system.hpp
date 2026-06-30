/**
 * @file 
 * @brief
 */

#pragma once
#include "core_engine_types.h"
#include <Metal/Metal.hpp>

class TransformationSystem
{
    public:
    
    /**
     * Add transformation to the Transformation system
     */
    TransformationHandle add(
        simd_float3 position,
        simd_quatf rotation,
        simd_float3 scale,
        TransformationHandle parent = NO_TRANSFORMATION_PARENT
    );

    /**
     * Add transformation to the Transformation system
     * @param transformation The transformation to be added
     * @param parent The parent of the transformation to be added. Defaults to `NO_TRANSFORMATION_PARENT`.
     * @returns The transformation handle of the transformation
     */
    TransformationHandle add(Transformation transformation, TransformationHandle parent = NO_TRANSFORMATION_PARENT);

    /**
     * Remove a transformation from the Transformation System
     * @param transformation The handle of the transformation to be removed.
     */
    void remove(TransformationHandle transformation);

    /**
     * Computes world matrices from the positions, rotations, and scales of the transformations within the system
     */
    void computeWorldMatrices();

    /**
     * Uploads the world matrices to the GPU
     */
    void uploadToGPU();

    MTL::Buffer* transformationBuffer = nullptr;
    
    std::vector<simd_float3> positions;
    std::vector<simd_quatf> rotations;
    std::vector<simd_float3> scales;
    std::vector<TransformationHandle> parentIndices;
    std::vector<matrix_float4x4> worldMatrices;
};