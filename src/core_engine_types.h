/**
 * @file core_engine_types.h
 * @brief Holds core types for the Deco Engine
 */

#pragma once
#include <simd/simd.h>
#include <memory>
#include <Metal/Metal.hpp>
#include "utils/AAPLMathUtilities.h"
#include <mutex>
#include "vertex.hpp"

#define DECO_ENGINE_LIST_TYPE(p_name, p_type) struct p_name { p_type *data; size_t count; \
    p_type& operator[](size_t index) const { return data[index]; } \
    p_type* begin() const { return data; } \
    p_type* end() const { return data + count; } \
};

using TransformationHandle = uint32_t;

inline constexpr TransformationHandle TRANSFORMATION_HANDLE_INVALID = UINT32_MAX;
inline constexpr TransformationHandle NO_TRANSFORMATION_PARENT = UINT32_MAX;

namespace Config
{
    inline constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 3;
    inline constexpr uint32_t MAX_TOTAL_BONES = 2048;
};

struct Transformation
{
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
};

template<typename T, typename H>
class SystemInputBuffer
{
    public:

    ~SystemInputBuffer<T, H>()
    {
        std::lock_guard<std::mutex> lock(mutex);
        delete[] buffer;
    }

    T* buffer = nullptr;
    H maxHandle;
    mutable std::mutex mutex;
    size_t count;
};

template<typename T>
class SystemOutputBuffer
{
    public:

    ~SystemOutputBuffer<T>()
    {
        std::lock_guard<std::mutex> lock(mutex);
        delete[] buffer;
    }

    T* buffer = nullptr;
    T largestHandle = 0;
    mutable std::mutex mutex;
    size_t count = 0;
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