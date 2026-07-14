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
    // TODO: Come up with a cleaner solution, like allowing for transformation addition requests to be queued up.
    {
        std::lock_guard<std::mutex> lock(renderThreadAdditionsInputBuffer.mutex);

        // Critical section
        if (renderThreadAdditionsInputBuffer.numberOfItems != 0)
        {
            // Allocating more memory for the buffer
            size_t oldSize = renderThreadAdditionsInputBuffer.numberOfItems;
            TransformationEntry* oldEntries = renderThreadAdditionsInputBuffer.buffer;
            renderThreadAdditionsInputBuffer.buffer = new TransformationEntry[oldSize + n];
            memcpy(renderThreadAdditionsInputBuffer.buffer, oldEntries, sizeof(TransformationEntry) * oldSize);
            delete[] oldEntries;

            // Filling the new memory
            memcpy(renderThreadAdditionsInputBuffer.buffer + oldSize, entries.data(), sizeof(TransformationEntry) * n);
        }
        else
        {
            if (renderThreadAdditionsInputBuffer.buffer)
                delete[] renderThreadAdditionsInputBuffer.buffer;
            renderThreadAdditionsInputBuffer.buffer = new TransformationEntry[n];
            renderThreadAdditionsInputBuffer.numberOfItems = n;
            memcpy(renderThreadAdditionsInputBuffer.buffer, entries.data(), n * sizeof(TransformationEntry));
        }
        return;
    }
}

void TransformationSystem::remove(TransformationHandle transformation)
{
    const uint32_t numberOfTransformations = (uint32_t)positions.size();
    remove(&transformation, 1);
}

void TransformationSystem::remove(TransformationHandle* handles, size_t n)
{
    std::lock_guard<std::mutex> lock(renderThreadRemovalsInputBuffer.mutex);
    
    // Critical section
    if (renderThreadRemovalsInputBuffer.numberOfItems != 0)
    {
        // Allocating more memory for the buffer
        size_t oldSize = renderThreadRemovalsInputBuffer.numberOfItems;
        TransformationHandle* oldHandles = renderThreadRemovalsInputBuffer.buffer;
        renderThreadRemovalsInputBuffer.buffer = new TransformationHandle[oldSize + n];
        memcpy(renderThreadRemovalsInputBuffer.buffer, oldHandles, sizeof(TransformationHandle) * oldSize);
        delete[] oldHandles;

        // Filling the new memory
        memcpy(renderThreadRemovalsInputBuffer.buffer + oldSize, handles, sizeof(TransformationHandle) * n);
    }
    else
    {
        if (renderThreadRemovalsInputBuffer.buffer)
            delete[] renderThreadRemovalsInputBuffer.buffer;
        renderThreadRemovalsInputBuffer.buffer = new TransformationHandle[n];
        renderThreadRemovalsInputBuffer.numberOfItems = n;
        memcpy(renderThreadRemovalsInputBuffer.buffer, handles, n * sizeof(TransformationHandle));
    }
}

void TransformationSystem::setParent(TransformationHandle transformation, TransformationHandle parent)
{
    TransformationReparentConfig* configs = new TransformationReparentConfig[1] {
        (TransformationReparentConfig){
            .child = transformation,
            .parent = parent}};
    setParents(configs, 1);
    delete[] configs;
}

void TransformationSystem::setParents(const TransformationReparentConfig* configs, size_t n)
{
    std::lock_guard<std::mutex> lock(renderThreadReparentInputBuffer.mutex);
    if (renderThreadReparentInputBuffer.numberOfItems == 0)
    {
        if (renderThreadReparentInputBuffer.buffer)
            delete[] renderThreadReparentInputBuffer.buffer;
        renderThreadReparentInputBuffer.buffer = new TransformationReparentConfig[n];
        memcpy(renderThreadReparentInputBuffer.buffer, configs, n * sizeof(TransformationReparentConfig));
        renderThreadReparentInputBuffer.numberOfItems = n;
    }
    else
    {
        const size_t oldSize = renderThreadReparentInputBuffer.numberOfItems;
        TransformationReparentConfig *newData = new TransformationReparentConfig[oldSize + n];
        // Using memcpy instead of memmove for speed
        memcpy(newData, renderThreadReparentInputBuffer.buffer, oldSize * sizeof(TransformationReparentConfig));
        memcpy(newData + oldSize, configs, n * sizeof(TransformationReparentConfig));
        delete[] renderThreadReparentInputBuffer.buffer;
        renderThreadReparentInputBuffer.buffer = newData;
        renderThreadReparentInputBuffer.numberOfItems = oldSize + n;
    }
}

inline matrix_float4x4 TransformationSystem::buildLocalMatrix(simd_float3 translation, simd_quatf rotation, simd_float3 scale)
{
    const matrix_float4x4 T = matrix4x4_translation(translation);
    const matrix_float4x4 R = simd_matrix4x4(rotation);
    const matrix_float4x4 S = matrix4x4_scale(scale);
    return simd_mul(T, simd_mul(R, S));
}

void TransformationSystem::computeWorldMatrices()
{
    for (uint32_t i = 0; i < positions.size(); ++i)
    {
        matrix_float4x4 local = buildLocalMatrix(positions[i], rotations[i], scales[i]);
        const TransformationHandle parentHandle = parentHandles[i];
        if (parentHandle == NO_TRANSFORMATION_PARENT)
        {
            worldMatrices[i] = local;
        } else {
            const uint32_t parentIndex = handleToIndex[parentHandle];
            worldMatrices[i] = simd_mul(worldMatrices[parentIndex], local);
        }
    }
}

void TransformationSystem::updateWorldMatrixBuffer(uint8_t bufferNumber)
{
    memcpy(transformationBuffers[bufferNumber]->contents(), worldMatrices.data(), worldMatrices.size() * sizeof(matrix_float4x4));
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
    
    for (size_t i = numOfFreeHandlesToTake; i < n; ++i)
    {
        handles[i] = (maxHandle);
        maxHandle += 1;
    }
    return handles;
}

void TransformationSystem::drainRenderThreadAdditionsInputBuffer()
{
    size_t n;
    TransformationEntry* queuedEntries;
    {
        std::lock_guard<std::mutex> lock(renderThreadAdditionsInputBuffer.mutex);
        // Critical section
        n = renderThreadAdditionsInputBuffer.numberOfItems;
        if (n == 0) return;
        queuedEntries = renderThreadAdditionsInputBuffer.buffer;
        renderThreadAdditionsInputBuffer.buffer = nullptr;
        renderThreadAdditionsInputBuffer.numberOfItems = 0;
    }
    const size_t lastIndex = positions.size() - 1;
    std::vector<TransformationHandle> consumedHandles;
    consumedHandles.reserve(n);

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
        consumedHandles.push_back(queuedEntries[i].handle);
    }

    std::lock_guard<std::mutex> lock(handlesConsumedByRenderThread.mutex);
    // Critical section
    size_t oldSize = handlesConsumedByRenderThread.numberOfItems;
    if (oldSize > 0)
    {
        TransformationHandle* temp = handlesConsumedByRenderThread.buffer;
        handlesConsumedByRenderThread.buffer = new TransformationHandle[oldSize + n];
        memcpy(handlesConsumedByRenderThread.buffer, temp, oldSize * sizeof(TransformationHandle));
        delete[] temp; // Deleting old buffer
        memcpy(handlesConsumedByRenderThread.buffer + oldSize, consumedHandles.data(), n * sizeof(TransformationHandle));
        handlesConsumedByRenderThread.numberOfItems = oldSize + n;
    }
    else
    {
        delete[] handlesConsumedByRenderThread.buffer; // Deleting old buffer
        handlesConsumedByRenderThread.buffer = new TransformationHandle[n];
        memcpy(handlesConsumedByRenderThread.buffer, consumedHandles.data(), n * sizeof(TransformationHandle));
        handlesConsumedByRenderThread.numberOfItems = n;
    }
}

void TransformationSystem::drainRenderThreadRemovalsInputBuffer()
{
    size_t n;
    TransformationHandle* queuedHandles;
    {
        std::lock_guard<std::mutex> lock(renderThreadRemovalsInputBuffer.mutex);
        // Critical section
        n = renderThreadRemovalsInputBuffer.numberOfItems;
        if (n == 0) return;
        queuedHandles = renderThreadRemovalsInputBuffer.buffer;
        renderThreadRemovalsInputBuffer.buffer = nullptr;
        renderThreadRemovalsInputBuffer.numberOfItems = 0;
    }

    // Getting the indices
    TransformationHandle queuedIndices[n];
    for (size_t i = 0; i < n; ++i)
    {
        queuedIndices[i] = handleToIndex[queuedHandles[i]];
    }

    // Sorting the indices
    std::sort(queuedIndices, queuedIndices + n);

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
        const uint32_t targetedIndex = queuedIndices[i];
        targetedIndices.push_back(targetedIndex);
        const uint32_t startIndex = targetedIndex + 1;
        memshiftTransformationsChunk(
            startIndex,
            -i - 1,
            queuedIndices[i + 1] - targetedIndex - 1
        );
    }

    // Removing the last thing
    const size_t lastIndexToRemove = queuedIndices[n - 1];
    targetedIndices.push_back(lastIndexToRemove);
    memshiftTransformationsChunk(
        lastIndexToRemove + 1,
        -n,
        indexToHandle.size() - lastIndexToRemove
    );

    positions.erase(positions.end() - n, positions.end());
    scales.erase(scales.end() - n, scales.end());
    rotations.erase(rotations.end() - n, rotations.end());
    worldMatrices.erase(worldMatrices.end() - n, worldMatrices.end());
    parentHandles.erase(parentHandles.end() - n, parentHandles.end());
    indexToHandle.erase(indexToHandle.end() - n, indexToHandle.end());

    // Updating mappings
    for (size_t i = queuedIndices[0]; i < indexToHandle.size(); ++i)
    {
        handleToIndex[indexToHandle[i]] = i;
    }

    // Filling the render thread removals output buffer
    if (renderThreadRemovalsOutputBuffer.size() > 0)
    {
        size_t oldSize;
        // If there's already data in the output buffer, append
        // free handles to the end
        TransformationHandle* previousFreeHandles;
        {
            std::lock_guard<std::mutex> lock(renderThreadRemovalsOutputBuffer.mutex);

            // Critical section
            oldSize = renderThreadRemovalsOutputBuffer.numberOfItems; // getting the size again, just in case it changed since the check.
            previousFreeHandles = renderThreadRemovalsOutputBuffer.buffer;
            renderThreadRemovalsOutputBuffer.buffer = nullptr;
            renderThreadRemovalsOutputBuffer.numberOfItems = 0;
        }
        size_t newSize = oldSize + n;
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
        std::lock_guard<std::mutex> lock(renderThreadRemovalsOutputBuffer.mutex);
        
        // Critical section
        if (renderThreadRemovalsOutputBuffer.buffer) delete[] renderThreadRemovalsOutputBuffer.buffer;
        renderThreadRemovalsOutputBuffer.buffer = new TransformationHandle[newSize];
        renderThreadRemovalsOutputBuffer.numberOfItems = newSize;
        memcpy(renderThreadRemovalsOutputBuffer.buffer, oldAndNewlyFreedHandles, oldSize * sizeof(TransformationHandle));
    } else {
        std::lock_guard<std::mutex> lock(renderThreadRemovalsOutputBuffer.mutex);
        
        // Critical section
        if (renderThreadRemovalsOutputBuffer.buffer) delete[] renderThreadRemovalsOutputBuffer.buffer;
        renderThreadRemovalsOutputBuffer.buffer = new TransformationHandle[n];
        renderThreadRemovalsOutputBuffer.numberOfItems = n;
        memcpy(renderThreadRemovalsOutputBuffer.buffer, queuedHandles, n * sizeof(TransformationHandle));
    }
    delete[] queuedHandles;
}

void TransformationSystem::drainRenderThreadReparentInputBuffer()
{
    TransformationReparentConfig* reparentConfigs;
    size_t numOfReparents = 0;
    {
        std::lock_guard<std::mutex> lock(renderThreadReparentInputBuffer.mutex);
        numOfReparents = renderThreadReparentInputBuffer.numberOfItems;
        if (numOfReparents == 0) return;
        reparentConfigs = new TransformationReparentConfig[numOfReparents];
        memcpy(reparentConfigs, renderThreadReparentInputBuffer.buffer, numOfReparents * sizeof(TransformationReparentConfig));
        delete[] renderThreadReparentInputBuffer.buffer;
        renderThreadReparentInputBuffer.buffer = nullptr;
        renderThreadReparentInputBuffer.numberOfItems = 0;
    }
    for (size_t i = 0; i < numOfReparents; ++i)
    {
        reparent(reparentConfigs[i]);
    }
    delete[] reparentConfigs;
}

void TransformationSystem::updateFreeHandles()
{
    size_t n;
    TransformationHandle* newlyFreedHandles;
    {
        std::lock_guard<std::mutex> lock(renderThreadRemovalsOutputBuffer.mutex);

        // Critical section
        n = renderThreadRemovalsOutputBuffer.numberOfItems;
        newlyFreedHandles = renderThreadRemovalsOutputBuffer.buffer;
        renderThreadRemovalsOutputBuffer.buffer = nullptr;
        renderThreadRemovalsOutputBuffer.numberOfItems = 0;
    }
    for (size_t i = 0; i < n; ++i)
    {
        freeHandles.push_back(newlyFreedHandles[i]);
    }
}

std::vector<TransformationHandle> TransformationSystem::drainAndGetConsumedTransformationHandles()
{
    std::lock_guard<std::mutex> lock(handlesConsumedByRenderThread.mutex);
    // Critical section
    std::vector<TransformationHandle> handles(handlesConsumedByRenderThread.buffer, handlesConsumedByRenderThread.buffer + handlesConsumedByRenderThread.numberOfItems);
    delete[] handlesConsumedByRenderThread.buffer;
    handlesConsumedByRenderThread.buffer = nullptr;
    handlesConsumedByRenderThread.numberOfItems = 0;
    return handles;
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
    handlesConsumedByRenderThread.~SynchronizedBuffer();
}

void TransformationSystem::deallocRenderThreadRemovalsOutputBuffer()
{
    renderThreadRemovalsOutputBuffer.~SynchronizedBuffer();
}

void TransformationSystem::memshiftTransformationsChunk(uint32_t startIndex, int shift, size_t size)
{
    if ((int32_t)startIndex + shift < 0)
    {
        shift = -startIndex;
    } 
    else if (startIndex == 0 && shift >= -1) 
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

// TODO: Support performing batches of reparent requests
void TransformationSystem::reparent(const TransformationReparentConfig config)
{
    const TransformationHandle transformation = config.child;
    const TransformationHandle parent = config.parent;
    parentHandles[handleToIndex[transformation]] = parent;
    if (parent != NO_TRANSFORMATION_PARENT)
    {
        const uint32_t childIndex = handleToIndex[transformation];
        const uint32_t parentIndex = handleToIndex[parent];
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
                    wallIndices[i - 1] - wallIndices[i]);
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
            for (uint32_t i = childIndex; i < parentIndex + 1; ++i)
            {
                if (indexToHandle[i] < handleToIndex.size())
                {
                    handleToIndex[indexToHandle[i]] = i;
                }
            }
        }
    }
}