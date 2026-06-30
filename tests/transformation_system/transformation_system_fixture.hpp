/**
 * @file transformation_system_fixture.hpp
 * @brief Helper functions for the transformation system tests
 */

#pragma once
#include "test_utils.hpp"
#include "transformation_system.hpp"

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
    return system;
}