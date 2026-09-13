/**
 * @file animation_system_fixture.hpp
 * @brief
 */

#pragma once
#include "core_systems/animation_system.hpp"

inline SkeletonDescription create_dummy_skeleton()
{
    return (SkeletonDescription)
    {
        .parent_indices = { NO_JOINT_PARENT },
        .rest_translations = { { 0, 0, 0 } },
        .rest_rotations = { simd_quaternion(simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)) },
        .rest_scales = { { 1, 1, 1 } },
        .joint_count = 1
    };
}

inline SkinBindingDescription create_skin_binding(SkeletonHandle handle)
{
    return (SkinBindingDescription)
    {
        .skeleton = handle,
        .inverse_bind_matrices = { matrix_identity_float4x4 }
    };
}