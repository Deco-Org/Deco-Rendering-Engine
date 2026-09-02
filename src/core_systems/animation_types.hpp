/**
 * @file animation_types.hpp
 * @brief 
 */

#pragma once
#include <vector>
#include <simd/simd.h>

using SkeletonHandle = uint32_t;
using SkinBindingHandle = uint32_t;
using ClipHandle = uint32_t;
using AnimationInstanceHandle = uint32_t;

static constexpr SkeletonHandle INVALID_SKELETON_HANDLE = UINT32_MAX;
static constexpr SkinBindingHandle INVALID_SKIN_BINDING_HANDLE = UINT32_MAX;
static constexpr ClipHandle INVALID_CLIP_HANDLE = UINT32_MAX;
static constexpr AnimationInstanceHandle INVALID_ANIMATION_INSTANCE_HANDLE = UINT32_MAX;
static constexpr uint32_t NO_JOINT_PARENT = UINT32_MAX;

struct SkeletonDescription
{
    std::vector<uint32_t> parent_indices;
    std::vector<simd_float3> rest_translations;
    std::vector<simd_quatf> rest_rotations;
    std::vector<simd_float3> rest_scales;
    uint32_t joint_count = 0;
};

struct SkinBindingDescription
{
    SkeletonHandle skeleton = INVALID_SKELETON_HANDLE;
    std::vector<matrix_float4x4> inverse_bind_matrices;
};

struct BakedKeyframe
{
    float time;
    simd_float3 translation;
    simd_quatf rotation;
    simd_float3 scale;
};

struct BakedJointTrack
{
    std::vector<BakedKeyframe> keyframes;
};

struct AnimationClipDescription
{
    SkeletonHandle skeleton = INVALID_SKELETON_HANDLE;
    float duration = 0.0f;
    std::vector<BakedJointTrack> joint_tracks;
};
