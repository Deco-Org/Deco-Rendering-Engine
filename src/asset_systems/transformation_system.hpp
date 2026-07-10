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
    TransformationHandle handle = TRANSFORMATION_HANDLE_INVALID;
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

    // /**
    //  * Add transformations to the Transformation system
    //  * @param transformations A pointer to transformation data to be added
    //  * @param parent A pointer to an array of parent handles
    //  * @param n The number of transformations to be added
    //  * @returns A vector of Transformation Handles
    //  */
    // std::vector<TransformationHandle> add(Transformation* transformations, TransformationHandle* parents, size_t n);

    /**
     * Add transformations to the Transformation system with pre-reserved handles
     * @param transformations A pointer to transformation data to be added
     * @param parent A pointer to the parent handles of each transformation
     * @param n The number of transformations to be added
     * @returns
     */
    void add(Transformation* transformations, TransformationHandle* parents, TransformationHandle* handles, size_t n);

    /**
     * Remove a transformation from the Transformation System
     * @param transformation The handle of the transformation to be removed.
     */
    void remove(TransformationHandle transformation);

    /**
     * Remove transformations from the Transformation System
     * @param handles An array of the handles of the transformations
     * to be removed
     * @param n The number of transformations to be removed.
     */
    void remove(TransformationHandle* handles, size_t n);

    void setParent(TransformationHandle transformation, TransformationHandle parent);

    /**
     * Computes world matrices from the positions, rotations, and scales of the transformations within the system
     */
    void computeWorldMatrices();

    /**
     * Updates the world matrix buffer
     */
    void updateWorldMatrixBuffer();

    std::vector<TransformationHandle> reserveHandles(size_t n);

    /**
     * @brief Drains the render thread additions input buffer, adding
     * transformation entries to the system.
     * @warning This should only be called on the render thread.
     */
    void drainRenderThreadAdditionsInputBuffer();

    /**
     * @brief Drains the render thread removals input buffer, removing
     * transformations with the provided handles from the system.
     * @warning This should only be called on the render thread.
     */
    void drainRenderThreadRemovalsInputBuffer();

    void deallocRenderThreadAdditionsInputBuffer();
    void deallocRenderThreadRemovalsInputBuffer();

    void deallocRenderThreadAdditionsOutputBuffer();
    void deallocRenderThreadRemovalsOutputBuffer();

    TransformationHandle getMaxHandle();

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
    void memshiftTransformationsChunk(uint32_t startIndex, size_t size, uint32_t shift);

    std::vector<TransformationHandle> freeHandles;
    TransformationHandle maxHandle = 0;

    // Input Buffers (loading thread to render thread)
    SynchronizedBuffer<TransformationEntry> renderThreadAdditionsInputBuffer = SynchronizedBuffer<TransformationEntry>();
    SynchronizedBuffer<TransformationHandle> renderThreadRemovalsInputBuffer = SynchronizedBuffer<TransformationHandle>();

    // Output Buffers (rener thread to loading thread)
    SynchronizedBuffer<TransformationHandle> renderThreadAdditionsOutputBuffer = SynchronizedBuffer<TransformationHandle>();
    SynchronizedBuffer<TransformationHandle> renderThreadRemovalsOutputBuffer = SynchronizedBuffer<TransformationHandle>();
};