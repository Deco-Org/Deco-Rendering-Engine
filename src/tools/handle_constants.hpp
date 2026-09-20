/**
 * @file handle_constants.hpp
 * @brief
 */

#pragma once

#include <stdint.h>
#include <cstddef>
#include "texture_loader.hpp"

using SubmeshHandle = uint32_t;
using MaterialHandle = uint16_t;
using TransformationHandle = uint32_t;
using RenderPipelineHandle = uint8_t;


template <typename T>
struct HandleConstants
{
    static constexpr const T invalid = static_cast<T>(-1);
};

#define DEFINE_INVALID_HANDLE(handle_type, invalid_handle) template <> struct HandleConstants<handle_type> { static constexpr const handle_type invalid = invalid_handle; };

DEFINE_INVALID_HANDLE(SubmeshHandle, -1);
DEFINE_INVALID_HANDLE(MaterialHandle, -1);
DEFINE_INVALID_HANDLE(TransformationHandle, -1);
DEFINE_INVALID_HANDLE(TextureLoader::TextureHandle, -1);
DEFINE_INVALID_HANDLE(RenderPipelineHandle, -1);

#undef DEFINE_INVALID_HANDLE;