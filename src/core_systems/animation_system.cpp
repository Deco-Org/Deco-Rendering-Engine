/**
 * @file animation_system.cpp
 * @brief 
 */

#include "animation_system.hpp"

AnimationSystem::AnimationSystem(MTL::Device *device = nullptr)
{
    
}

SkeletonHandle AnimationSystem::add_skeleton(SkeletonDescription description)
{
    
}

SkinBindingDescription AnimationSystem::add_skin_binding(SkinBindingDescription description)
{
    
}

ClipHandle AnimationSystem::add_clip(AnimationClipDescription description)
{
    
}

AnimationInstanceHandle AnimationSystem::add_instance(SkeletonHandle skeleton, SkinBindingHandle skin_binding)
{
    
}

void AnimationSystem::remove_instance(AnimationInstanceHandle instance)
{
    
}

void AnimationSystem::play(AnimationInstanceHandle instance, ClipHandle clip, const bool loop = false)
{
    
}

void AnimationSystem::stop(AnimationInstanceHandle instance)
{
    
}

void AnimationSystem::update(float delta_time)
{
    
}

void AnimationSystem::update_skinning_buffer(uint8_t buffer_index)
{
    
}

NS::UInteger AnimationSystem::skinning_buffer_offset(AnimationInstanceHandle instance) const
{
    
}

simd_quatf AnimationSystem::interpolate_rotation(simd_quatf q0, simd_quatf q1, float t)
{
    
}

void AnimationSystem::sample_track(
    const BakedJointTrack& track, 
    float time, 
    simd_float3& out_transformation,
    simd_float3& out_rotation,
    simd_float3& out_scale)
{
    
}