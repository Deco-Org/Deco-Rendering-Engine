/**
 * @file 
 * @brief
 */

#pragma once
#include "core_engine_types.h"
#include "tools/synchronized_buffer.hpp"
#include <Metal/Metal.hpp>

struct TransformationEntry
{
    Transformation transformation = {0};
    TransformationHandle parent = NO_TRANSFORMATION_PARENT;
};

class TransformationSystem
{
    public:

    TransformationSystem(MTL::Device* device = nullptr);

    TransformationSystem(TransformationSystem&&) = default;

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

    void setParent(TransformationHandle transformation, TransformationHandle parent);

    /**
     * Computes world matrices from the positions, rotations, and scales of the transformations within the system
     */
    void computeWorldMatrices();

    /**
     * Updates the world matrix buffer
     */
    void updateWorldMatrixBuffer();

    void deallocRenderThreadAdditionsInputBuffer();
    void deallocRenderThreadRemovalsInputBuffer();

    void deallocRenderThreadAdditionsOutputBuffer();
    void deallocRenderThreadRemovalsOutputBuffer();

    // MTL::Buffer* transformationBuffer = nullptr;
    MetalBufferPtr transformationBuffer = nullptr;
    
    std::vector<simd_float3> positions;
    std::vector<simd_quatf> rotations;
    std::vector<simd_float3> scales;

    std::vector<TransformationHandle> parentHandles;
    std::vector<matrix_float4x4> worldMatrices;
    
    // Maps handles to the indices in the arrays
    std::vector<uint32_t> handleToIndex;
    
    // Maps indices in the arrays to handles
    std::vector<TransformationHandle> indexToHandle;

    private:
    std::vector<TransformationHandle> freeHandles;

    // Input Buffers (loading thread to render thread)
    SynchronizedBuffer<TransformationEntry> renderThreadAdditionsInputBuffer = SynchronizedBuffer<TransformationEntry>();
    SynchronizedBuffer<TransformationEntry> renderThreadRemovalsInputBuffer = SynchronizedBuffer<TransformationEntry>();

    // Output Buffers (rener thread to loading thread)
    SynchronizedBuffer<TransformationHandle> renderThreadAdditionsOutputBuffer = SynchronizedBuffer<TransformationHandle>();
    SynchronizedBuffer<TransformationHandle> renderThreadRemovalsOutputBuffer = SynchronizedBuffer<TransformationHandle>();
};