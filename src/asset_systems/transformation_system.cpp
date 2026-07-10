/**
 * @file transformation_system.cpp
 * @brief 
 */

#include "transformation_system.hpp"

TransformationSystem::TransformationSystem(MTL::Device* device)
{
    if (device) 
    {
        transformationBuffer.reset(device->newBuffer(sizeof(matrix_float4x4), MTL::ResourceStorageModeShared));
    }
}

TransformationHandle TransformationSystem::add(Transformation transformation, TransformationHandle parent)
{
    TransformationHandle handle;
    if (freeHandles.size() > 0)
    {
        handle = freeHandles.back();
        freeHandles.pop_back();
    } else {
        handle = (TransformationHandle)positions.size();
    }
    positions.push_back(transformation.position);
    rotations.push_back(transformation.rotation);
    scales.push_back(transformation.scale);
    parentHandles.push_back(parent);

    uint32_t index;
    if (handle > handleToIndex.size())
    {
        // If the handle is brand new, go ahead and expand handToIndex
        index = (uint32_t)(handleToIndex.size());
        handleToIndex.push_back(index);
    }
    handleToIndex.push_back((uint32_t)handle);
    indexToHandle.push_back(handle);
    return handle;
}

void TransformationSystem::remove(TransformationHandle transformation)
{
    const uint32_t numberOfTransformations = (uint32_t)positions.size();
    // Everything after the removed transformation must be shifted
    // The mappings between handle and index must also be updated
    for (TransformationHandle t = transformation; t < positions.size() - 1; ++t)
    {
        positions[t] = positions[t+1];
        rotations[t] = rotations[t+1];
        scales[t] = scales[t+1];

        const TransformationHandle newHandle = indexToHandle[t + 1];
        handleToIndex[newHandle] = t;
        indexToHandle[t] = newHandle;
    }
    positions.pop_back();
    rotations.pop_back();
    scales.pop_back();
    freeHandles.push_back(transformation);
}

void TransformationSystem::deallocRenderThreadAdditionsInputBuffer()
{
    renderThreadAdditionsInputBuffer.~SynchronizedBuffer();
}

void TransformationSystem::deallocRenderThreadAdditionsOutputBuffer()
{
    renderThreadAdditionsOutputBuffer.~SynchronizedBuffer();
}