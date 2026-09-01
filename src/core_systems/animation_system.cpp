/**
 * @file animation_system.cpp
 * @brief 
 */

#include "animation_system.hpp"
#include <utility>

AnimationSystem::AnimationSystem(MTL::Device *device = nullptr)
{
    if (device)
    {
        for (uint8_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; ++i)
        {
            skinning_buffers[i] = device->newBuffer(
                Config::MAX_TOTAL_BONES * sizeof(matrix_float4x4), 
                MTL::ResourceStorageModeShared);
        }
    }
}

AnimationSystem::~AnimationSystem()
{
    for (uint8_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (skinning_buffers[i])
        {
            skinning_buffers[i]->release();
            skinning_buffers[i] = nullptr;
        }
    }
}

SkeletonHandle AnimationSystem::add_skeleton(SkeletonDescription description)
{
    skeletons.push_back(std::move(description));
    return skeletons.size() - 1;
}

SkinBindingHandle AnimationSystem::add_skin_binding(SkinBindingDescription description)
{
    skin_bindings.push_back(std::move(description));
    return skin_bindings.size() - 1;
}

ClipHandle AnimationSystem::add_clip(AnimationClipDescription description)
{
    clips.push_back(std::move(description));
    return clips.size() - 1;
}

AnimationInstanceHandle AnimationSystem::add_instance(SkeletonHandle skeleton, SkinBindingHandle skin_binding)
{
    AnimationInstance instance;
    instance.skeleton = skeleton;    
    instance.skin_binding = skin_binding;

    uint32_t joint_count = skeletons[skeleton].joint_count;
    instance.joint_model_matrices.resize(joint_count);
    instance.skinning_matrices.resize(joint_count);

    // should check range has val, waiting for log util
    auto range = skinning_allocator.alloc(joint_count);
    instance.skinning_range = *range;

    if (!free_instance_handles.empty())
    {
        AnimationInstanceHandle handle = free_instance_handles.back();
        free_instance_handles.pop_back();
        instances[handle] = std::move(instance);
        return handle;
    }

    instances.push_back(std::move(instance));
    return instances.size() - 1;
}

void AnimationSystem::remove_instance(AnimationInstanceHandle instance)
{
    skinning_allocator.free(instances[instance].skinning_range);
    instances[instance].skeleton = INVALID_SKELETON_HANDLE;
    free_instance_handles.push_back(instance);
}

void AnimationSystem::play(AnimationInstanceHandle instance, ClipHandle clip, const bool loop = false)
{
    auto& context = instances[instance];
    context.clip = clip;
    context.current_time = 0.0f;
    context.loops = loop;
    context.is_playing = true;
}

void AnimationSystem::stop(AnimationInstanceHandle instance)
{
    instances[instance].is_playing = false;
}

void AnimationSystem::set_playback_speed(AnimationInstanceHandle instance, const float speed)
{
    instances[instance].playback_speed = speed;
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