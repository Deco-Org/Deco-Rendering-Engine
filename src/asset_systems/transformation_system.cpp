/**
 * @file transformation_system.cpp
 * @brief 
 */

#include "transformation_system.hpp"
#include <ranges>
#include <algorithm>
#include <type_traits>

TransformationSystem::TransformationSystem(MTL::Device* device)
{
    if (device) 
    {
        for (uint8_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; ++i) 
        {
            transformationBuffers[i].reset(device->newBuffer(sizeof(matrix_float4x4), MTL::ResourceStorageModeShared));
        }
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
    remove(&transformation, 1);
}

void TransformationSystem::remove(TransformationHandle* handles, size_t n)
{
    assert(renderThreadRemovalsInputBuffer.size() == 0);

    renderThreadRemovalsInputBuffer.setSize(n);
    renderThreadRemovalsInputBuffer.fillData(handles, n);
}

void TransformationSystem::setParent(TransformationHandle transformation, TransformationHandle parent)
{
    const uint32_t childIndex = handleToIndex[transformation];
    const uint32_t parentIndex = handleToIndex[parent];
    parentHandles[transformation] = parent;

    if (childIndex < parentIndex)
    {
        std::vector<simd_float3> tempPositions = {positions[parentIndex]};
        std::vector<simd_quatf> tempRotations = {rotations[parentIndex]};
        std::vector<simd_float3> tempScales = {scales[parentIndex]};
        std::vector<TransformationHandle> tempParentHandles = {parentHandles[parentIndex]};
        std::vector<TransformationHandle> tempIndexToHandle = {indexToHandle[parentIndex]};

        // Getting the "wall indices" (includes ancestors)
        std::vector<uint32_t> wallIndices{parentIndex};
        TransformationHandle ancestorHandle = parentHandles[parentIndex];
        while (ancestorHandle != NO_TRANSFORMATION_PARENT && handleToIndex[ancestorHandle] > childIndex)
        {
            const uint32_t ancestorIndex = handleToIndex[ancestorHandle];
            wallIndices.push_back(ancestorIndex);
            ancestorHandle = parentHandles[ancestorIndex];

            // Adding to temp arrays
            tempPositions.push_back(positions[ancestorIndex]);
            tempRotations.push_back(rotations[ancestorIndex]);
            tempScales.push_back(scales[ancestorIndex]);
            tempParentHandles.push_back(parentHandles[ancestorIndex]);
            tempIndexToHandle.push_back(indexToHandle[ancestorIndex]);
        }
        wallIndices.push_back(childIndex);
        
        const uint32_t numOfWalls = wallIndices.size();
        // Shift chunks over
        for (uint32_t i = 1; i < numOfWalls; ++i)
        {
            memshiftTransformationsChunk(
                wallIndices[i],
                i,
                wallIndices[i - 1] - wallIndices[i]
            );
        }

        // Putting the temps back
        const uint32_t newChildIndex = childIndex + numOfWalls - 1;
        for (uint32_t i = 0; i < numOfWalls - 1; ++i)
        {
            const uint32_t newIndex = newChildIndex - i - 1;
            positions[newIndex] = tempPositions[i];
            rotations[newIndex] = tempRotations[i];
            scales[newIndex] = tempScales[i];
            parentHandles[newIndex] = tempParentHandles[i];
            indexToHandle[newIndex] = tempIndexToHandle[i];
        }

        // Updating the mappings
        for (uint32_t i = 0; i < indexToHandle.size(); ++i)
        {
            if (indexToHandle[i] < handleToIndex.size())
            {
                handleToIndex[indexToHandle[i]] = i;
            }
        }
    }
}

std::vector<TransformationHandle> TransformationSystem::reserveHandles(size_t n)
{
    const size_t numOfFreeHandlesToTake = std::min(freeHandles.size(), n);
    std::vector<TransformationHandle> handles;
    handles.resize(n);
    if (freeHandles.size() > 0) {
        memcpy(
            handles.data(), 
            freeHandles.data() + (freeHandles.size() - numOfFreeHandlesToTake),
            numOfFreeHandlesToTake * sizeof(TransformationHandle));
        
        freeHandles.erase(freeHandles.end() - numOfFreeHandlesToTake, freeHandles.end());
    }
    
    const TransformationHandle lastHandle = n - numOfFreeHandlesToTake;
    for (size_t i = numOfFreeHandlesToTake; i < n; ++i)
    {
        handles[i] = (maxHandle);
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

        // Assigning handle mappings
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
        const uint32_t targetedIndex = handleToIndex[queuedHandles[i]];
        targetedIndices.push_back(targetedIndex);
        memshiftTransformationsChunk(
            handleToIndex[queuedHandles[i]] + 1,
            -i - 1,
            queuedHandles[i+1] - queuedHandles[i] - 1
        );
    }

    // Removing the last thing
    targetedIndices.push_back(handleToIndex[queuedHandles[n - 1]]);
    const size_t lastIndexToRemove = handleToIndex[queuedHandles[n - 1]];
    memshiftTransformationsChunk(
        lastIndexToRemove + 1,
        -n,
        positions.size() - lastIndexToRemove
    );

    positions.erase(positions.end() - n, positions.end());
    scales.erase(scales.end() - n, scales.end());
    rotations.erase(rotations.end() - n, rotations.end());
    worldMatrices.erase(worldMatrices.end() - n, worldMatrices.end());
    parentHandles.erase(parentHandles.end() - n, parentHandles.end());
    indexToHandle.erase(indexToHandle.end() - n, indexToHandle.end());

    // Updating mappings
    for (size_t i = 0; i < indexToHandle.size(); ++i)
    {
        handleToIndex[indexToHandle[i]] = i;
    }

    // Filling the render thread removals output buffer
    if (renderThreadRemovalsOutputBuffer.size() > 0)
    {
        // If there's already data in the output buffer, append
        // free handles to the end
        const size_t oldSize = renderThreadRemovalsOutputBuffer.size();
        size_t newSize = oldSize + n;
        TransformationHandle* previousFreeHandles = renderThreadRemovalsOutputBuffer.moveData();
        TransformationHandle oldAndNewlyFreedHandles[newSize];
        
        memcpy(oldAndNewlyFreedHandles, previousFreeHandles, oldSize);
        size_t numberOfNewlyFreedHandles = 0;
        // Counting the number of times a handle in the buffer is equal to a newly freed handle.
        for (size_t i = 0; i < oldSize; ++i)
        {
            if (std::ranges::find(queuedHandles, queuedHandles + n, i) != queuedHandles + n)
            {
                newSize -= 1;
            } else {
                oldAndNewlyFreedHandles[numberOfNewlyFreedHandles + oldSize] = previousFreeHandles[i];
                numberOfNewlyFreedHandles += 1;
            }
        }
        // Filling the buffer.
        // It might be better to get rid of the "thread safe buffer" type altogether and just do manual lockings
        renderThreadRemovalsOutputBuffer.setSize(newSize);
        renderThreadRemovalsOutputBuffer.fillData(oldAndNewlyFreedHandles, oldSize);
    } else {
        renderThreadRemovalsOutputBuffer.setSize(n);
        renderThreadRemovalsOutputBuffer.fillData(queuedHandles, n);
    }
}

void TransformationSystem::updateFreeHandles()
{
    const size_t n = renderThreadRemovalsOutputBuffer.size();
    TransformationHandle* newlyFreedHandles = renderThreadRemovalsOutputBuffer.moveData();
    for (size_t i = 0; i < n; ++i)
    {
        freeHandles.push_back(newlyFreedHandles[i]);
    }
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

void TransformationSystem::memshiftTransformationsChunk(uint32_t startIndex, int shift, size_t size)
{
    if (startIndex == 0 && shift < -1)
    {
        startIndex += 1;
        shift += 1;
    } else if (startIndex == 0 && shift >= -1) 
    {
        return;
    }

    // TODO: Look into future optimizations made possible through SIMD
    memmove(&positions[startIndex + shift], &positions[startIndex], size * sizeof(positions[0]));
    memmove(&rotations[startIndex + shift], &rotations[startIndex], size * sizeof(rotations[0]));
    memmove(&scales[startIndex + shift], &scales[startIndex], size * sizeof(scales[0]));
    memmove(&worldMatrices[startIndex + shift], &worldMatrices[startIndex], size * sizeof(worldMatrices[0]));
    memmove(&parentHandles[startIndex + shift], &parentHandles[startIndex], size * sizeof(parentHandles[0]));
    memmove(&indexToHandle[startIndex + shift], &indexToHandle[startIndex], size * sizeof(indexToHandle[0]));
}