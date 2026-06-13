// 
// vertex_data.hpp
//
// Created on 30 May 2026
// 

#pragma once
#include <simd/simd.h>

using namespace simd;

struct VertexData
{
    float3 position;
    float3 normal;
};

struct TransformationData
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
    float4x4 normalMatrix; // Important for nonuniform scaling and shearing
};