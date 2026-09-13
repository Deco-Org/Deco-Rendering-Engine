/**
 * @file animation_importer_test.cpp
 * @brief
 */

#include <catch2/catch_test_macros.hpp>
#include "asset_systems/animation_importer.hpp"
#include "animation_importer_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("topological sort orders parents before children", "[animation][sort]")
{
    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("assets/test_fall.fbx", nullptr, &error);
    REQUIRE(scene != nullptr);
    REQUIRE(scene->skin_deformers.count > 0);

    ufbx_skin_deformer *skin = scene->skin_deformers[0];

    auto joints = AnimationImporter::topologically_sort_joints(skin);
    REQUIRE(joints.size() == skin->clusters.count);

    for (size_t i = 0; i < joints.size(); ++i)
    {
        if (joints[i]->parent)
        {
            size_t parent_index = find_joint_index(joints, joints[i]->parent);
            
            if (parent_index != static_cast<size_t>(-1))
            {
                CHECK(parent_index < i);
            }
        }
    }

    ufbx_free_scene(scene);
}

TEST_CASE("import skeleton gets valid rest poses aligned to joint order", "[animation][skeleton]")
{
    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("assets/test_fall.fbx", nullptr, &error);
    REQUIRE(scene != nullptr);
    REQUIRE(scene->skin_deformers.count > 0);

    ufbx_skin_deformer *skin = scene->skin_deformers[0];

    auto joints = AnimationImporter::topologically_sort_joints(skin);
    SkeletonDescription skeleton_description = AnimationImporter::import_skeleton(skin);

    REQUIRE(skeleton_description.joint_count == joints.size());
    CHECK(skeleton_description.parent_indices.size() == skeleton_description.joint_count);
    CHECK(skeleton_description.rest_translations.size() == skeleton_description.joint_count);
    CHECK(skeleton_description.rest_rotations.size() == skeleton_description.joint_count);
    CHECK(skeleton_description.rest_scales.size() == skeleton_description.joint_count);

    bool has_root = false;

    for (size_t i = 0; i < skeleton_description.parent_indices.size(); ++i)
    {
        uint32_t parent_index = skeleton_description.parent_indices[i];

        if (parent_index == NO_JOINT_PARENT)
        {
            has_root = true;
        }
        else
        {
            CHECK(parent_index < i);
        }
    }

    CHECK(has_root);

    ufbx_free_scene(scene);
}

TEST_CASE("import skin binding maps inverse bind matrices exactly to joint count", "[animation][binding]")
{
    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("assets/test_fall.fbx", nullptr, &error);
    REQUIRE(scene != nullptr);
    REQUIRE(scene->skin_deformers.count > 0);

    ufbx_skin_deformer *skin = scene->skin_deformers[0];

    auto joints = AnimationImporter::topologically_sort_joints(skin);
    SkeletonHandle skeleton = 4;
    SkinBindingDescription binding_description = AnimationImporter::import_skin_binding(skin, joints, skeleton);

    CHECK(binding_description.skeleton == skeleton);
    REQUIRE(binding_description.inverse_bind_matrices.size() == joints.size());

    bool all_have_transforms = true;

    for (const auto& m : binding_description.inverse_bind_matrices)
    {
        if (simdMatrix4x4Equal(m, matrix_identity_float4x4))
        {
            all_have_transforms = false;
            break;
        }
    }

    CHECK(all_have_transforms);

    ufbx_free_scene(scene);
}

TEST_CASE("import_clip clamps un-animated tracks to guarantee uniform sampling", "[animation][clip]") 
{
    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file("assets/test_fall.fbx", nullptr, &error);
    REQUIRE(scene != nullptr);
    REQUIRE(scene->skin_deformers.count > 0);
    REQUIRE(scene->anim_stacks.count > 0);

    ufbx_skin_deformer* skin = scene->skin_deformers[0];

    auto joints = AnimationImporter::topologically_sort_joints(skin);
    SkeletonHandle skeleton = 4;
    
    AnimationClipDescription clip = AnimationImporter::import_clip(scene, scene->anim_stacks[0], joints, skeleton);

    CHECK(clip.duration > 0.0f);
    REQUIRE(clip.joint_tracks.size() == joints.size());

    size_t expected_keys = clip.joint_tracks[0].keyframes.size();
    REQUIRE(expected_keys > 0);

    for (size_t i = 1; i < clip.joint_tracks.size(); ++i) 
    {
        CHECK(clip.joint_tracks[i].keyframes.size() == expected_keys);
    }

    ufbx_free_scene(scene);
}