/**
 * @file animation_system.cpp
 * @brief 
 */

#include "animation_system.hpp"
#include <utility>
#include <AAPLMathUtilities.h>

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

void AnimationSystem::sample_track(
    const BakedJointTrack& track, 
    float time, 
    simd_float3& out_translation,
    simd_quatf& out_rotation,
    simd_float3& out_scale)
{
    // fixed fps for now, shoud look into dynamic if worth it
    const float fps = 60.0f;
    float exact_frame = time * fps;
    int frame0 = (int)std::floor(exact_frame);
    int frame1 = frame0 + 1;
    float t = exact_frame - (float)frame0;

    int max_frame = (int)track.keyframes.size() - 1;
    frame0 = std::clamp(frame0, 0, max_frame);
    frame1 = std::clamp(frame1, 0, max_frame);

    const BakedKeyframe& k0 = track.keyframes[frame0];
    const BakedKeyframe& k1 = track.keyframes[frame1];

    out_translation = simd_mix(k0.translation, k1.translation, t);
    out_rotation = quaternion_nlerp(k0.rotation, k1.rotation, t);
    out_scale = simd_mix(k0.scale, k1.scale, t);
}

void AnimationSystem::evaluate_joint(
    AnimationInstance& instance,
    const AnimationClipDescription& clip,
    const SkeletonDescription& skeleton,
    const SkinBindingDescription& binding,
    uint32_t joint_index)
{
    simd_float3 translation;
    simd_quatf rotation;
    simd_float3 scale;

    sample_track(
        clip.joint_tracks[joint_index], 
        instance.current_time, 
        translation,
        rotation,
        scale);

    matrix_float4x4 local_matrix = matrix4x4_trs(translation, rotation, scale);

    uint32_t joint_parent = skeleton.parent_indices[joint_index];

    if (joint_parent == NO_JOINT_PARENT)
    {
        instance.joint_model_matrices[joint_index] = local_matrix;
    }
    else
    {
        instance.joint_model_matrices[joint_index] = 
            simd_mul(
                instance.joint_model_matrices[joint_parent], 
                local_matrix);
    }

    instance.skinning_matrices[joint_index] = 
        simd_mul(
            instance.joint_model_matrices[joint_index], 
            binding.inverse_bind_matrices[joint_index]);
}

void AnimationSystem::update(float delta_time)
{
    for (AnimationInstance& instance : instances)
    {
        if (instance.skeleton == INVALID_SKELETON_HANDLE || !instance.is_playing) continue;

        const AnimationClipDescription& clip = clips[instance.clip];
        const SkeletonDescription& skeleton = skeletons[instance.skeleton];
        const SkinBindingDescription& binding = skin_bindings[instance.skin_binding];

        instance.current_time += delta_time * instance.playback_speed;

        if (instance.loops)
        {
            instance.current_time = std::fmod(instance.current_time, clip.duration);
        }
        else if (instance.current_time >= clip.duration)
        {
            instance.current_time = clip.duration;
            instance.is_playing = false;
        }

         for (uint32_t i = 0; i < skeleton.joint_count; ++i)
        {
            evaluate_joint(instance, clip, skeleton, binding, i);
        }
    }
}

void AnimationSystem::update_skinning_buffer(uint8_t buffer_index)
{
    matrix_float4x4 *buffer_data = static_cast<matrix_float4x4*>(skinning_buffers[buffer_index]->contents());

    for (AnimationInstance& instance : instances)
    {
        if (instance.skeleton == INVALID_SKELETON_HANDLE || !instance.is_playing)
        {
            continue;
        }

        memcpy(
            buffer_data + instance.skinning_range.start,
            instance.skinning_matrices.data(),
            instance.skinning_matrices.size() * sizeof(matrix_float4x4));
    }
}

NS::UInteger AnimationSystem::get_skinning_buffer_offset(AnimationInstanceHandle instance) const
{
    return instances[instance].skinning_range.start * sizeof(matrix_float4x4);
}