/**
 * @file transformation_system.cpp
 * @brief 
 */

#include "transformation_system.hpp"

TransformationSystem::TransformationSystem(MTL::Device* device)
{
    if (device) 
    {
        transformationBuffer = device->newBuffer(sizeof(matrix_float4x4), MTL::ResourceStorageModeShared);
    }
}

TransformationSystem::~TransformationSystem()
{
    transformationBuffer->release();
    transformationBuffer = nullptr;
}

TransformationHandle TransformationSystem::add(Transformation transformation, TransformationHandle parent)
{
    const TransformationHandle handle = positions.size();
    positions.push_back(transformation.position);
    rotations.push_back(transformation.rotation);
    scales.push_back(transformation.scale);
    parentIndices.push_back(parent);
    return handle;
}

void TransformationSystem::remove(TransformationHandle transformation)
{
    
}