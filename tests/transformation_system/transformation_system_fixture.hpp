/**
 * @file transformation_system_fixture.hpp
 * @brief Helper functions for the transformation system tests
 */

#pragma once
#include "test_utils.hpp"
#include <utility>
#include "asset_systems/transformation_system.hpp"

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
                // if (i > 4)
                // {
                    parents[i] = handles[i - (i % 4)];
                    break;
                    // }
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