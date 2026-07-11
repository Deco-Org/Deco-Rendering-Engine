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
                .rotation = (simd_quatf) { { rotationComponent, rotationComponent, rotationComponent, rotationComponent } },
                .scale = (simd_float3) { (float)i / n * 8, (float)i / n * 8, (float)i / n * 8 }
            });
    }
    return transformations;
}

inline TransformationSystem makeTransformationSystemWithNTransformations(uint32_t n)
{
    TransformationSystem system;
    std::vector<Transformation> transformations = nTransformations(n);
    std::vector<TransformationHandle> parents(n, NO_TRANSFORMATION_PARENT);
    std::vector<TransformationHandle> handles = system.reserveHandles(n);
    system.add(transformations.data(), parents.data(), handles.data(), n);
    system.drainRenderThreadAdditionsInputBuffer();
    return std::move(system);
}

inline std::vector<TransformationEntry> nUnparentedTransformationEntries(TransformationSystem &system, uint32_t n)
{
    std::vector<TransformationHandle> handles = system.reserveHandles(n);
    std::vector<TransformationEntry> entries = std::vector<TransformationEntry>();
    for (uint32_t i = 0; i < n; ++i)
    {
        const float rotationComponent = (float)(i) / n;
        entries.push_back((TransformationEntry){
            .transformation = {
                .position = (simd_float3){(float)i, (float)i, (float)i},
                .rotation = (simd_quatf){{rotationComponent, rotationComponent, rotationComponent, rotationComponent == 0.0f ? 0.1f : rotationComponent}},
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
    // for (TransformationHandle i = 0; i < system.getMaxHandle(); ++i)
    // {
    //     if (system.handleToIndex[i] != (uint32_t)(-1))
    //     {
    //         REQUIRE(system.indexToHandle[system.handleToIndex[i]] == i);
    //     }
    // }
}

inline void renderThreadInputQueueIsCleared(TransformationSystem &system)
{
    system.deallocRenderThreadAdditionsInputBuffer();
}