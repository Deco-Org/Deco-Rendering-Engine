/**
 * @file animation_system_test.cpp
 * @brief
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core_systems/animation_system.hpp"
#include "animation_system_fixture.hpp"

TEST_CASE("adding and removing instances recycles handles", "[animation]")
{
    AnimationSystem sys(nullptr);
    SkeletonHandle skeleton = sys.add_skeleton(create_dummy_skeleton());

    auto a = sys.add_instance(skeleton, INVALID_SKIN_BINDING_HANDLE);
    (void)sys.add_instance(skeleton, INVALID_SKIN_BINDING_HANDLE);

    sys.remove_instance(a);

    auto c = sys.add_instance(skeleton, INVALID_SKIN_BINDING_HANDLE);
    REQUIRE(c == a);
}

TEST_CASE("one shot animation stops correctly", "[animation][playback]")
{
    AnimationSystem sys(nullptr);
    SkeletonHandle skeleton = sys.add_skeleton(create_dummy_skeleton());
    SkinBindingHandle binding = sys.add_skin_binding(create_skin_binding(skeleton));
    AnimationInstanceHandle instance = sys.add_instance(skeleton, binding);

    AnimationClipDescription clip_description = { .skeleton = skeleton, .duration = 5.0f };
    clip_description.joint_tracks.resize(1);
    ClipHandle clip = sys.add_clip(clip_description);

    sys.play(instance, clip, false);

    sys.update(5.001f);

    REQUIRE_THAT(sys.get_current_time(instance), Catch::Matchers::WithinAbs(5.0f, 0.0001f));
    REQUIRE_FALSE(sys.is_playing(instance));
}

TEST_CASE("looping animation wraps time using mod", "[animation][playback]")
{
    AnimationSystem sys(nullptr);
    SkeletonHandle skeleton = sys.add_skeleton(create_dummy_skeleton());
    SkinBindingHandle binding = sys.add_skin_binding(create_skin_binding(skeleton));
    AnimationInstanceHandle instance = sys.add_instance(skeleton, binding);

    AnimationClipDescription clip_description = { .skeleton = skeleton, .duration = 5.0f };
    clip_description.joint_tracks.resize(1);
    ClipHandle clip = sys.add_clip(clip_description);

    sys.play(instance, clip, true);
    sys.update(5.001f);

    REQUIRE_THAT(sys.get_current_time(instance), Catch::Matchers::WithinAbs(0.001f, 0.0001f));
    REQUIRE(sys.is_playing(instance));
}

TEST_CASE("playback speed scales time advancement correctly", "[animation][playback]")
{
    AnimationSystem sys(nullptr);
    SkeletonHandle skeleton = sys.add_skeleton(create_dummy_skeleton());
    SkinBindingHandle binding = sys.add_skin_binding(create_skin_binding(skeleton));
    AnimationInstanceHandle instance = sys.add_instance(skeleton, binding);

    AnimationClipDescription clip_description = { .skeleton = skeleton, .duration = 5.0f };
    clip_description.joint_tracks.resize(1);
    ClipHandle clip = sys.add_clip(clip_description);

    sys.play(instance, clip, false);
    sys.set_playback_speed(instance, 0.5f);
    sys.update(1.0f);

    REQUIRE_THAT(sys.get_current_time(instance), Catch::Matchers::WithinAbs(0.5f, 0.0001f));
}

TEST_CASE("skinning buffer offset should align to allocator start index", "[animation][memory]")
{
    AnimationSystem sys(nullptr);
    SkeletonDescription skeleton_description = create_dummy_skeleton();
    skeleton_description.joint_count = 10;
    SkeletonHandle skeleton = sys.add_skeleton(skeleton_description);

    (void)sys.add_instance(skeleton, INVALID_SKIN_BINDING_HANDLE);
    auto b = sys.add_instance(skeleton, INVALID_SKIN_BINDING_HANDLE);

    REQUIRE(sys.get_skinning_buffer_offset(b) == 10 * sizeof(matrix_float4x4));
}