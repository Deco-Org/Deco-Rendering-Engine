/**
 * @file animation_system.hpp
 * @brief 
 */

#pragma once
#include "animation_types.hpp"
#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "tools/range_allocator.hpp"

struct AnimationInstance
{
    SkeletonHandle skeleton = INVALID_SKELETON_HANDLE;
    SkinBindingHandle skin_binding = INVALID_SKIN_BINDING_HANDLE;
    ClipHandle clip = INVALID_CLIP_HANDLE;
    float current_time = 0.0f;
    float playback_speed = 1.0f;
    bool is_playing = false;
    bool loops = false;
    AllocationRange skinning_range{ 0, 0 };
    std::vector<matrix_float4x4> joint_model_matrices;
    std::vector<matrix_float4x4> skinning_matrices;
};

class AnimationSystem
{
public:
    AnimationSystem(MTL::Device *device = nullptr);
    ~AnimationSystem();

    SkeletonHandle add_skeleton(SkeletonDescription description);
    SkinBindingHandle add_skin_binding(SkinBindingDescription description);
    ClipHandle add_clip(AnimationClipDescription description);
    AnimationInstanceHandle add_instance(SkeletonHandle skeleton, SkinBindingHandle skin_binding);
    void remove_instance(AnimationInstanceHandle instance);

    void play(AnimationInstanceHandle instance, ClipHandle clip, const bool loop = false);
    void set_playback_speed(AnimationInstanceHandle instance, const float speed);
    void stop(AnimationInstanceHandle instance);
    
    void update(float delta_time);
    
    void update_skinning_buffer(uint8_t buffer_index);
    NS::UInteger get_skinning_buffer_offset(AnimationInstanceHandle instance) const;

private:
    static void sample_track(
        const BakedJointTrack& track, 
        float time, 
        simd_float3& out_transformation,
        simd_quatf& out_rotation,
        simd_float3& out_scale);
    static void evaluate_joint(
        AnimationInstance& instance,
        const AnimationClipDescription& clip,
        const SkeletonDescription& skeleton,
        const SkinBindingDescription& binding,
        const uint32_t joint_index);

    std::vector<SkeletonDescription> skeletons;
    std::vector<SkinBindingDescription> skin_bindings;
    std::vector<AnimationClipDescription> clips;
    std::vector<AnimationInstance> instances;
    std::vector<AnimationInstanceHandle> free_instance_handles;

    RangeAllocator skinning_allocator{Config::MAX_TOTAL_BONES};
    MTL::Buffer *skinning_buffers[Config::MAX_FRAMES_IN_FLIGHT] = {};
};