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

inline TransformationSystem makeTransformationSystemWithNTransformations(uint32_t n)
{
    TransformationSystem system;
    for (uint32_t i = 0; i < n; ++i)
    {
        const float rotationComponent = (float)i / n;
        TransformationHandle handle = system.add(
            (Transformation) {
                .position = (simd_float3) { (float)i, (float)i, (float)i },
                .rotation = (simd_quatf) { { rotationComponent, rotationComponent, rotationComponent, rotationComponent } },
                .scale = (simd_float3) { (float)i / n * 8, (float)i / n * 8, (float)i / n * 8 }
            },
            NO_TRANSFORMATION_PARENT
        );
    }
    return std::move(system);
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