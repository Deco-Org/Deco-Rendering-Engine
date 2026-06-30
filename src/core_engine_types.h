/**
 * @file core_engine_types.h
 * @brief Holds core types for the Deco Engine
 */

#pragma once
#include <simd/simd.h>

using TransformationHandle = uint32_t;

inline constexpr TransformationHandle NO_TRANSFORMATION_PARENT = UINT32_MAX;

struct Transformation
{
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
};