/**
 * @file transformation_system.cpp
 * @brief 
 */

#include "transformation_system.hpp"
#include <ranges>
#include <algorithm>

TransformationSystem::TransformationSystem(MTL::Device* device)
{
    if (device) 
    {
        transformationBuffer.reset(device->newBuffer(sizeof(matrix_float4x4), MTL::ResourceStorageModeShared));
    }
}


TransformationHandle TransformationSystem::add(Transformation transformation, TransformationHandle parent)
{
    TransformationHandle handle = reserveHandles(1)[0];
    add(&transformation, &parent, &handle, 1);
    return handle;
}

void TransformationSystem::add(Transformation* transformations, TransformationHandle* parents, TransformationHandle* handles, size_t n)
{
    // Queuing entries to be sent to the render thread
    std::vector<TransformationEntry> entries;
    entries.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        entries.push_back(
            (TransformationEntry){
                .transformation = transformations[i],
                .parent = parents[i],
                .handle = handles[i]});
    }

    // The render thread additions input buffer must be empty before new transformations can be added
    assert(renderThreadAdditionsInputBuffer.size() == 0);

    renderThreadAdditionsInputBuffer.setSize(n);
    renderThreadAdditionsInputBuffer.fillData(entries.data(), n);
}

void TransformationSystem::remove(TransformationHandle transformation)
{
    const uint32_t numberOfTransformations = (uint32_t)positions.size();
    // Everything after the removed transformation must be shifted
    // The mappings between handle and index must also be updated
    // for (TransformationHandle t = transformation; t < positions.size() - 1; ++t)
    // {
    //     positions[t] = positions[t+1];
    //     rotations[t] = rotations[t+1];
    //     scales[t] = scales[t+1];

    //     const TransformationHandle newHandle = indexToHandle[t + 1];
    //     handleToIndex[newHandle] = t;
    //     indexToHandle[t] = newHandle;
    // }
    // positions.pop_back();
    // rotations.pop_back();
    // scales.pop_back();
    // freeHandles.push_back(transformation);
}

void TransformationSystem::remove(TransformationHandle* handles, size_t n)
{
    assert(renderThreadRemovalsInputBuffer.size() == 0);

    renderThreadRemovalsInputBuffer.setSize(n);
    renderThreadRemovalsInputBuffer.fillData(handles, n);
}

std::vector<TransformationHandle> TransformationSystem::reserveHandles(size_t n)
{
    size_t numOfFreeHandlesToTake = std::max(freeHandles.size(), n);
    std::vector<TransformationHandle> handles;
    handles.reserve(n);
    std::ranges::move(freeHandles | std::views::take(numOfFreeHandlesToTake), std::back_inserter(handles));
    
    const TransformationHandle lastHandle = numOfFreeHandlesToTake - handles.size();
    for (size_t i = handles.size(); i < lastHandle; ++i)
    {
        handles.push_back(maxHandle);
        maxHandle += 1;
    }
    return handles;
}

void TransformationSystem::drainRenderThreadAdditionsInputBuffer()
{
    const size_t n = renderThreadAdditionsInputBuffer.size();
    if (n == 0) return;
    TransformationEntry* queuedEntries = renderThreadAdditionsInputBuffer.moveData();
    const size_t lastIndex = positions.size() - 1;

    // Allocating new memory if needed
    if (positions.size() + n < positions.capacity())
    {
        positions.reserve(lastIndex + 1 + n);
        rotations.reserve(lastIndex + 1 + n);
        scales.reserve(lastIndex + 1 + n);
        worldMatrices.reserve(lastIndex + 1 + n);
        parentHandles.reserve(lastIndex + 1 + n);
    }

    // The entries in the render thread should already be sorted such that
    // parents come before children, and unparented nodes come first
    for (size_t i = 0; i < n; ++i)
    {
        positions.push_back(queuedEntries[i].transformation.position);
        rotations.push_back(queuedEntries[i].transformation.rotation);
        scales.push_back(queuedEntries[i].transformation.scale);
        parentHandles.push_back(queuedEntries[i].parent);
        worldMatrices.push_back({0}); // probably should be replaced

        // // Assigning handle mappings
        if (queuedEntries[i].handle >= handleToIndex.size())
        {
            handleToIndex.resize(queuedEntries[i].handle + 1, NO_TRANSFORMATION_PARENT);
        }
        handleToIndex[queuedEntries[i].handle] = lastIndex + 1 + i;
        if (lastIndex + 1 + i >= indexToHandle.size())
        {
            indexToHandle.resize(lastIndex + 1 + i + 1, NO_TRANSFORMATION_PARENT);
        }
        indexToHandle[lastIndex + 1 + i] = queuedEntries[i].handle;
    }
}

void TransformationSystem::drainRenderThreadRemovalsInputBuffer()
{
    const size_t n = renderThreadRemovalsInputBuffer.size();
    TransformationHandle* queuedHandles = renderThreadRemovalsInputBuffer.moveData();

    // TODO: Future optimization: move orphans first
    // For now, just move things in big chunks of memory
    std::vector<uint32_t> targetedIndices;
    targetedIndices.reserve(n);

    // Removing a transformation:
    //  Move everything between the two items that are being removed
    //  by k spaces, where k is the number of items that have been
    //  removed so far.
    size_t removalCount;
    for (size_t i = 0; i < n - 1; ++i)
    {
        targetedIndices.push_back(handleToIndex[queuedHandles[i]]);
        memshiftTransformationsChunk(
            handleToIndex[queuedHandles[i]],
            queuedHandles[i+1] - queuedHandles[i],
            i
        );
    }
    // Removing the last thing
    targetedIndices.push_back(handleToIndex[queuedHandles[n - 1]]);
    const size_t lastIndexToRemove = handleToIndex[queuedHandles[n - 1]];
    memshiftTransformationsChunk(
        lastIndexToRemove,
        positions.size() - lastIndexToRemove,
        n
    );

    positions.pop_back();
    scales.pop_back();
    rotations.pop_back();
    worldMatrices.pop_back();
    parentHandles.pop_back();
}

void TransformationSystem::deallocRenderThreadAdditionsInputBuffer()
{
    renderThreadAdditionsInputBuffer.~SynchronizedBuffer();
}

void TransformationSystem::deallocRenderThreadRemovalsInputBuffer()
{
    renderThreadRemovalsInputBuffer.~SynchronizedBuffer();
}

void TransformationSystem::deallocRenderThreadAdditionsOutputBuffer()
{
    renderThreadAdditionsOutputBuffer.~SynchronizedBuffer();
}

void TransformationSystem::deallocRenderThreadRemovalsOutputBuffer()
{
    renderThreadRemovalsOutputBuffer.~SynchronizedBuffer();
}

TransformationHandle TransformationSystem::getMaxHandle() 
{
    return maxHandle;
}

void TransformationSystem::memshiftTransformationsChunk(uint32_t startIndex, size_t size, uint32_t shift)
{
    // TODO: Look into future optimizations made possible through SIMD
    memmove(&positions[startIndex] - shift, &positions[startIndex], size * sizeof(positions[0]));
    memmove(&rotations[startIndex] - shift, &rotations[startIndex], size * sizeof(rotations[0]));
    memmove(&scales[startIndex] - shift, &scales[startIndex], size * sizeof(scales[0]));
    memmove(&worldMatrices[startIndex] - shift, &worldMatrices[startIndex], size * sizeof(worldMatrices[0]));
    memmove(&parentHandles[startIndex] - shift, &parentHandles[startIndex], size * sizeof(parentHandles[0]));
    
    // Updating mappings
    for (size_t i = 0; i < size - shift; ++i)
    {
        const TransformationHandle oldHandle = indexToHandle[i + startIndex];
        indexToHandle[i + startIndex] = indexToHandle[i + startIndex + shift];
        handleToIndex[oldHandle] = indexToHandle[i + startIndex];
    }
}