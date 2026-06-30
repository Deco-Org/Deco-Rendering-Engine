#pragma once
#include <simd/simd.h>

bool simdFloat3Equal(simd_float3 a, simd_float3 b, float epsilon = 0.0001f) {
    return std::abs(a[0] - b[0]) < epsilon &&
           std::abs(a[1] - b[1]) < epsilon &&
           std::abs(a[2] - b[2]) < epsilon;
}

bool simdQuatfEqual(simd_quatf a, simd_quatf b, float epsilon = 0.0001f) {
    return std::abs(a.vector[0] - b.vector[0]) < epsilon &&
           std::abs(a.vector[1] - b.vector[1]) < epsilon &&
           std::abs(a.vector[2] - b.vector[2]) < epsilon &&
           std::abs(a.vector[3] - b.vector[3]) < epsilon;
}