/**
 * @file vertex.hpp
 * @brief Contains vertex types
 */

#pragma once
#include <simd/simd.h>

struct Vertex
{
    simd::float3 position;
    simd::float3 normal;
    simd::float2 uv;
};