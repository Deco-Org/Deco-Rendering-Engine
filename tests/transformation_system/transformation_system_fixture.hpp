/**
 * @file transformation_system_fixture.hpp
 * @brief Helper functions for the transformation system tests
 */

#pragma once
#include "test_utils.hpp"
#include <utility>
#include <functional>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include "asset_systems/transformation_system.hpp"

using OperationQueue = std::queue<std::function<void()>>;

constexpr simd_float3 originPosition = { 0.0f, 0.0f, 0.0f };
constexpr simd_quatf zeroQuaternion = { { 0.0f, 0.0f, 0.0f, 0.0f } };
constexpr simd_float3 defaultScale = { 1.0f, 1.0f, 1.0f };

constexpr simd_float3 somePosition = { 1.0f, 3.0f, 12.0f };
constexpr simd_quatf someRotation = { { 0.3f, 0.5f, 0.9f, 0.8f } };
constexpr simd_float3 someScale = { 0.5f, 2.0f, 3.6f };

inline std::vector<Transformation> nTransformations(uint32_t n)
{
    std::vector<Transformation> transformations = std::vector<Transformation>();
    for (uint32_t i = 0; i < n; ++i)
    {
        const float rotationComponent = (float)i / n;
        transformations.push_back(
            (Transformation) {
                .position = (simd_float3) { (float)i, (float)i, (float)i },
                .rotation = (simd_quatf) { { rotationComponent, rotationComponent, rotationComponent, rotationComponent == 0.0f ? 1.0f : rotationComponent } },
                .scale = (simd_float3) { (float)i / n * 8, (float)i / n * 8, (float)i / n * 8 }
            });
    }
    return transformations;
}

inline TransformationSystem makeTransformationSystemWithNUnparentedTransformations(uint32_t n)
{
    TransformationSystem system;
    std::vector<Transformation> transformations = nTransformations(n);
    std::vector<TransformationHandle> parents(n, NO_TRANSFORMATION_PARENT);
    std::vector<TransformationHandle> handles = system.reserveHandles(n);
    system.add(transformations.data(), parents.data(), handles.data(), n);
    system.drainRenderThreadAdditionsInputBuffer();
    return std::move(system);
}

inline TransformationSystem makeTransformationSystemWithNTransformations(uint32_t n)
{
    TransformationSystem system;
    std::vector<Transformation> transformations = nTransformations(n);
    std::vector<TransformationHandle> handles = system.reserveHandles(n);
    std::vector<TransformationHandle> parents(n, NO_TRANSFORMATION_PARENT);
    
    // Generating parents
    for (size_t i = 0; i < n; ++i)
    {
        switch (i % 4)
        {
            case 1:
            {
                parents[i] = handles[i - (i % 4)];
                break;
            }
            case 2:
            {
                if (i > 4) {
                    parents[i] = handles[i - (i / 4)];
                    break;
                }
            }
                
            case 3:
            {
                if (i >= 16)
                {
                    parents[i] = handles[i - 16];
                    break;
                }
            }

            default:
            parents[i] = NO_TRANSFORMATION_PARENT;
        }
    }

    // Sanity check
    for (size_t i = 0; i < n; ++i)
    {
        assert((
            parents[i] == NO_TRANSFORMATION_PARENT ||
            parents[i] < handles[i]
        ));
    }

    system.add(transformations.data(), parents.data(), handles.data(), n);
    system.drainRenderThreadAdditionsInputBuffer();
    return std::move(system);
}

inline std::vector<TransformationEntry> nUnparentedTransformationEntries(TransformationSystem& system, uint32_t n)
{
    std::vector<TransformationHandle> handles = system.reserveHandles(n);
    std::vector<TransformationEntry> entries = std::vector<TransformationEntry>();
    for (uint32_t i = 0; i < n; ++i)
    {
        const float rotationComponent = (float)(i) / n;
        entries.push_back((TransformationEntry){
            .transformation = {
                .position = (simd_float3){(float)i, (float)i, (float)i},
                .rotation = (simd_quatf){{rotationComponent, rotationComponent, rotationComponent, rotationComponent == 0.0f ? 1.0f : rotationComponent}},
                .scale = (simd_float3){(float)i / n * 8, (float)i / n * 8, (float)i / n * 8}},
            .parent = NO_TRANSFORMATION_PARENT,
            .handle = handles[i]});
    }
}

inline TransformationHandle someTransformationHandleForAddedTransformation(
    TransformationSystem &system,
    Transformation transformation = (Transformation){
        .position = somePosition,
        .rotation = someRotation,
        .scale = someScale},
    TransformationHandle parent = NO_TRANSFORMATION_PARENT)
{
    return system.add(transformation);
}

inline void renderThreadInputQueueIsCleared(TransformationSystem& system)
{
    system.deallocRenderThreadAdditionsInputBuffer();
}

void someBatchesOfTransformationsAreAdded(TransformationSystem& system, std::vector<std::vector<Transformation>> batches)
{
    for (size_t i = 0; i < batches.size(); ++i)
    {
        std::vector<TransformationHandle> handles = system.reserveHandles(batches[i].size());
        std::vector<TransformationHandle> parents(batches[i].size(), NO_TRANSFORMATION_PARENT);
        system.add(batches[i].data(), parents.data(), handles.data(), batches[i].size());
        system.updateFreeHandles();
    }
}

void someBatchesOfTransformationsAreRemoved(TransformationSystem& system, std::vector<std::vector<TransformationHandle>> batches)
{
    for (size_t i = 0; i < batches.size(); ++i)
    {
        std::vector<TransformationHandle>& batch = batches[i];
        system.remove(batch.data(), batch.size());
    }
}

void someBatchesOfTransformationsAreReparented(TransformationSystem& system, std::vector<std::vector<TransformationReparentConfig>> batches)
{
    for (size_t i = 0; i < batches.size(); ++i)
    {
        system.setParents(batches[i].data(), batches[i].size());
    }
}

void someTransformationsAreAddedAndTheRenderThreadSuccessfullyAddsThem(
    TransformationSystem& system, 
    std::vector<TransformationHandle>& liveHandles, 
    unsigned int numToAdd)
{
    std::vector<TransformationHandle> reservedHandles = system.reserveHandles(numToAdd);
    // Generating transformations
    std::vector<Transformation> transformations = nTransformations(numToAdd);
    std::vector<TransformationHandle> parents(numToAdd, NO_TRANSFORMATION_PARENT);
    system.add(
        transformations.data(),
        parents.data(),
        reservedHandles.data(),
        numToAdd
    );
    liveHandles.insert(liveHandles.end(), reservedHandles.begin(), reservedHandles.end());
}

/**
 * Extra care must be taken to make sure that transformations have no children before they
 * are removed, as is the case with real use of the transformation system. This greatly
 * complicates this test function.
 */
void someTransformationsAreRemovedAndTheRenderThreadSuccessfullyRemovesThem(
    TransformationSystem& system,
    std::vector<TransformationHandle>& liveHandles,
    std::unordered_map<TransformationHandle, int>& parentToNumberOfChildrenMap,
    std::unordered_map<TransformationHandle, TransformationHandle>& childrenToParentMap,
    unsigned int& numUniqueParents,
    unsigned int numToRemove,
    std::mutex& ackMutex,
    std::condition_variable& ackCv,
    bool& removalComplete)
{
    std::vector<TransformationHandle> transformationsToRemove;
    transformationsToRemove.reserve(numToRemove);
    // Generating array of handles to remove
    for (unsigned int i = 0; i < numToRemove; ++i)
    {
        TransformationHandle targetHandle = liveHandles[i * (liveHandles.size() / numToRemove)];
        
        // Making sure the handle being removed has no children
        const TransformationHandle originalTarget = targetHandle;
        auto it = parentToNumberOfChildrenMap.find(targetHandle);
        int numberOfChildren = (it == parentToNumberOfChildrenMap.end()) ? 0 : it->second;
        while (targetHandle <= liveHandles.size() && numberOfChildren != 0)
        {
            targetHandle += 1;
            it = parentToNumberOfChildrenMap.find(targetHandle);
            if (it == parentToNumberOfChildrenMap.end())
            {
                numberOfChildren = 0;
            }
            else
            {
                numberOfChildren = it->second;
            }
        }
        if (targetHandle == liveHandles.size())
        {
            // Continuing to search from the beginning
            targetHandle = 0;
            it = parentToNumberOfChildrenMap.find(targetHandle);
            numberOfChildren = (it == parentToNumberOfChildrenMap.end()) ? 0 : it->second;
            while (targetHandle <= originalTarget && numberOfChildren != 0)
            {
                targetHandle += 1;
                it = parentToNumberOfChildrenMap.find(targetHandle);
                numberOfChildren = (it == parentToNumberOfChildrenMap.end()) ? 0 : it->second;
            }
            // If no handles are found, assert
            assert(targetHandle != originalTarget);
        }

        // Remove the handle
        if (childrenToParentMap.contains(targetHandle))
        {
            parentToNumberOfChildrenMap[childrenToParentMap[targetHandle]] -= 1;
            if (parentToNumberOfChildrenMap[childrenToParentMap[targetHandle]] == 0)
            {
                numUniqueParents = (numUniqueParents == 0) ? 0 : numUniqueParents - 1;
            }
        }
        transformationsToRemove.push_back(targetHandle);
    }

    // Removing handles
    system.remove(
        transformationsToRemove.data(),
        transformationsToRemove.size()
    );
    {
        std::unique_lock<std::mutex> lock(ackMutex);
        ackCv.wait(lock, [&]{ return removalComplete; });
        removalComplete = false;
    }
    // Updating liveHandles
    std::unordered_set<TransformationHandle> removeSet(transformationsToRemove.begin(), transformationsToRemove.end());
    std::erase_if(liveHandles, [&removeSet](int x) {
        return removeSet.contains(x);
    });

    // Updating internal test parent-child mappings
    const TransformationHandle parent = childrenToParentMap[parent];
    parentToNumberOfChildrenMap[parent] -= 1;
}

/**
 * Extra care must be taken to make sure that transformations have no children before they
 * are removed, as is the case with real use of the transformation system. This greatly
 * complicates this test function.
 */
void someTransformationsAreReparentedAndTheRenderThreadSuccessfullyReparentsThem(
    TransformationSystem& system,
    std::vector<TransformationHandle>& liveHandles,
    std::unordered_map<TransformationHandle, int>& parentToNumberOfChildrenMap,
    std::unordered_map<TransformationHandle, TransformationHandle>& childrenToParentMap,
    unsigned int numToReparent,
    std::mutex& ackMutex,
    std::condition_variable& ackCv,
    bool& reparentingComplete,
    std::mt19937_64& randomEngine,
    unsigned int& numUniqueParents,
    unsigned int& maxUniqueParents)
{
    std::vector<TransformationReparentConfig> reparentConfigs;
    std::uniform_int_distribution<TransformationHandle> distrib(
        *std::min_element(liveHandles.begin(), liveHandles.end()), 
        *std::max_element(liveHandles.begin(), liveHandles.end()));
    for (unsigned int i = 0; i < numToReparent; ++i)
    {
        
        TransformationReparentConfig config;
        config.child = liveHandles[liveHandles.size() / numToReparent];
        config.parent = config.child;
        while (config.parent == config.child)
        {
            config.parent = distrib(randomEngine);
        }
        if (numUniqueParents == maxUniqueParents)
        {
            // If the maximum number of unique parents has been reached,
            // loop over free handles until a transformation with children is found
            TransformationHandle targetParent = liveHandles[0];
            for (size_t i = 0; i < liveHandles.size(); ++i)
            {
                auto it = parentToNumberOfChildrenMap.find(targetParent);
                if (it != parentToNumberOfChildrenMap.end() && it->second > 0)
                {
                    parentToNumberOfChildrenMap[targetParent] += 1;
                    break;
                }
                else
                    continue;
            }
            config.parent = targetParent;
        }
        else
        {
            auto it = parentToNumberOfChildrenMap.find(config.parent);
            if (it == parentToNumberOfChildrenMap.end())
            {
                parentToNumberOfChildrenMap[config.parent] = 0;
                numUniqueParents += 1;
            }
            parentToNumberOfChildrenMap[config.parent] += 1;
        }
        reparentConfigs.push_back(config);
        
        // Updating internal test mappings (used to make sure no parents are removed)
        const TransformationHandle oldParent = childrenToParentMap[config.child];
        childrenToParentMap[config.child] = config.parent;
        parentToNumberOfChildrenMap[oldParent] += 1;
        parentToNumberOfChildrenMap[config.parent] += 1;
    }

    // Reparenting
    system.setParents(reparentConfigs.data(), reparentConfigs.size());
    {
        std::unique_lock<std::mutex> lock(ackMutex);
        ackCv.wait(lock, [&]{ return reparentingComplete; });
        reparentingComplete = false;
    }
}

void worldMatricesAreComputedNTimes(TransformationSystem& system, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
    {
        system.drainRenderThreadRemovalsInputBuffer();
        system.drainRenderThreadAdditionsInputBuffer();
        system.drainRenderThreadReparentInputBuffer();
        system.computeWorldMatrices();
    }
}

void worldMatricesAreComputedUntilDone(
    TransformationSystem &system,
    std::atomic<bool>& done,
    std::mutex& ackMutex,
    std::condition_variable& ackCv,
    bool& removalComplete,
    bool& reparentingComplete)
{
    
    while (!done)
    {
        // Removals
        system.drainRenderThreadRemovalsInputBuffer();
        {
            std::lock_guard<std::mutex> lock(ackMutex);
            removalComplete = true;
        }
        ackCv.notify_all();

        // Additions
        system.drainRenderThreadAdditionsInputBuffer();

        // Reparents
        system.drainRenderThreadReparentInputBuffer();
        {
            std::lock_guard<std::mutex> lock(ackMutex);
            reparentingComplete = true;
        }
        ackCv.notify_all();
    }
}

void runOperations(OperationQueue& ops)
{
    while (!ops.empty())
    {
        auto op = ops.front();
        ops.pop();
        op();
    }
}

inline void numberOfTransformationsShouldBe(TransformationSystem& system, size_t n)
{
    REQUIRE(n == system.positions.size());
    REQUIRE(n == system.rotations.size());
    REQUIRE(n == system.scales.size());
}

// if using multithreading, this should only be called on the render thread
inline void handlesAndIndicesShouldMatchUp(TransformationSystem& system)
{
    std::vector<TransformationHandle> handles;
    std::vector<size_t> indices;
    for (size_t i = 0; i < system.positions.size(); ++i)
    {
        if (system.indexToHandle[i] != (uint32_t)(-1))
        {
            REQUIRE(system.handleToIndex[system.indexToHandle[i]] == i);
        }
    }
}

inline void allParentsShouldComeBeforeChildren(TransformationSystem& system)
{
    for (uint32_t i = 0; i < system.positions.size(); ++i)
    {
        const TransformationHandle parentHandle = system.parentHandles[i];
        if (parentHandle != NO_TRANSFORMATION_PARENT)
        {
            const uint32_t parentIndex = system.handleToIndex[parentHandle];
            REQUIRE(parentIndex < i);
        }
    }
}