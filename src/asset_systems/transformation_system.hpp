/**
 * @file 
 * @brief
 */

#pragma once
#include "core_engine_types.h"
#include "tools/synchronized_buffer.hpp"
#include "utils/AAPLMathUtilities.h"
#include <Metal/Metal.hpp>

struct TransformationEntry
{
    Transformation transformation = {0};
    TransformationHandle parent = NO_TRANSFORMATION_PARENT;
    TransformationHandle handle = TRANSFORMATION_HANDLE_INVALID;
};

struct TransformationReparentConfig
{
    TransformationHandle child = TRANSFORMATION_HANDLE_INVALID;
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
     * @brief Queue reparent requests to reparent transformations in
     * the transformation system.
     * @param configs An array of transformation reparent configs
     * @param n The number of transformation reparent configs.
     */
    void setParents(const TransformationReparentConfig const* configs, size_t n);

    /**
     * Computes world matrices from the positions, rotations, and scales of the transformations within the system
     * @warning This should only be called on the render thread.
     */
    void computeWorldMatrices();

    /**
     * Updates the world matrix buffer
     * @param bufferNumber The buffer to update
     * @warning This should only be called on the render thread.
     */
    void updateWorldMatrixBuffer(uint8_t bufferNumber);

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

    /**
     * @brief Drains the render thread reparent input buffer,
     * reparenting transformations according to the reparent configs
     * passed in.
     * @warning This should only be called on the render thread.
     */
    void drainRenderThreadReparentInputBuffer();

    /**
     * @brief Drains the render thread removals output buffer,
     * filling the free handles list with newly freed handles
     */
    void updateFreeHandles();

    void deallocRenderThreadAdditionsInputBuffer();
    void deallocRenderThreadRemovalsInputBuffer();

    void deallocRenderThreadAdditionsOutputBuffer();
    void deallocRenderThreadRemovalsOutputBuffer();

    TransformationHandle getMaxHandle();

    // MTL::Buffer* transformationBuffer = nullptr;
    MetalBufferPtr transformationBuffers[Config::MAX_FRAMES_IN_FLIGHT] = {nullptr, nullptr, nullptr};
    
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
    void memshiftTransformationsChunk(uint32_t startIndex, int shift, size_t size);
    void reparent(TransformationReparentConfig config);
    inline matrix_float4x4 buildLocalMatrix(simd_float3 translation, simd_quatf rotation, simd_float3 scale);

    std::vector<TransformationHandle> freeHandles;
    TransformationHandle maxHandle = 0;

    // Input Buffers (loading thread to render thread)
    SynchronizedBuffer<TransformationEntry> renderThreadAdditionsInputBuffer;
    SynchronizedBuffer<TransformationHandle> renderThreadRemovalsInputBuffer;
    SynchronizedBuffer<TransformationReparentConfig> renderThreadReparentInputBuffer;

    // Output Buffers (rener thread to loading thread)
    SynchronizedBuffer<TransformationHandle> renderThreadAdditionsOutputBuffer;
    SynchronizedBuffer<TransformationHandle> renderThreadRemovalsOutputBuffer;
    SynchronizedBuffer<const TransformationReparentConfig> renderThreadReparentOutputBuffer;
};