/**
 * @file core_engine_types.h
 * @brief Holds core types for the Deco Engine
 */

#pragma once
#include <simd/simd.h>
#include <memory>
#include <Metal/Metal.hpp>

using TransformationHandle = uint32_t;

inline constexpr TransformationHandle TRANSFORMATION_HANDLE_INVALID = UINT32_MAX;
inline constexpr TransformationHandle NO_TRANSFORMATION_PARENT = UINT32_MAX;

namespace Config
{
    inline constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 3;
};

struct Transformation
{
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
};

struct BufferDeleter
{
    void operator()(MTL::Buffer* buffer) const
    {
        if (buffer)
        {
            buffer->release();
        }
    }
};

using MetalBufferPtr = std::unique_ptr<MTL::Buffer, BufferDeleter>;