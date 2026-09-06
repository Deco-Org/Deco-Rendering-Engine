/**
 * @file animation_importer.cpp
 * @brief 
 */

#include "animation_importer.hpp"
#include <algorithm>

std::vector<ufbx_node*> AnimationImporter::topologically_sort_joints(ufbx_skin_deformer *skin)
{
    std::vector<ufbx_node*> joints;
    joints.reserve(skin->clusters.count);

    for (size_t i = 0; i < skin->clusters.count; ++i)
    {
        joints.push_back(skin->clusters[i]->bone_node);
    }

    auto get_bone_node_depth = [](ufbx_node *node)
    {
        int depth = 0;
        while (node->parent)
        {
            node = node->parent;
            depth++;
        }
        return depth;
    };

    std::stable_sort(joints.begin(), joints.end(), [&](ufbx_node *n1, ufbx_node *n2)
    {
        return get_bone_node_depth(n1) < get_bone_node_depth(n2);
    });

    return joints;
}

void AnimationImporter::import_joint(
    SkeletonDescription &description,
    const std::vector<ufbx_node*>& joints,
    size_t joint_index)
{
    ufbx_node *joint = joints[joint_index];
    auto parent_it = std::find(joints.begin(), joints.end(), joint->parent);

    if (parent_it != joints.end())
    {
        description.parent_indices[joint_index] = 
            static_cast<uint32_t>(std::distance(joints.begin(), parent_it));
    }
    else
    {
        description.parent_indices[joint_index] = NO_JOINT_PARENT;
    }

    ufbx_transform local = joint->local_transform;

    description.rest_translations[joint_index] =
        simd_make_float3(
            static_cast<float>(local.translation.x),
            static_cast<float>(local.translation.y),
            static_cast<float>(local.translation.z));

    description.rest_rotations[joint_index] =
        simd_quaternion(
            static_cast<float>(local.rotation.x),
            static_cast<float>(local.rotation.y),
            static_cast<float>(local.rotation.z),
            static_cast<float>(local.rotation.w));

    description.rest_scales[joint_index] =
        simd_make_float3(
            static_cast<float>(local.scale.x),
            static_cast<float>(local.scale.y),
            static_cast<float>(local.scale.z));
}

SkeletonDescription AnimationImporter::import_skeleton(ufbx_skin_deformer *skin)
{
    std::vector<ufbx_node*> joints = topologically_sort_joints(skin);

    SkeletonDescription description;
    description.joint_count = joints.size();
    description.parent_indices.resize(joints.size());
    description.rest_translations.resize(joints.size());
    description.rest_rotations.resize(joints.size());
    description.rest_scales.resize(joints.size());

    for (size_t i = 0; i < joints.size(); ++i)
    {
        import_joint(description, joints, i);
    }

    return description;
}

static ufbx_skin_cluster *find_skin_cluster(const ufbx_skin_cluster_list& skin_clusters, const ufbx_node *joint)
{
    for (size_t i = 0; i < skin_clusters.count; ++i)
    {
        if (skin_clusters[i]->bone_node == joint)
        {
            return skin_clusters[i];
        }
    }

    return nullptr;
}

SkinBindingDescription AnimationImporter::import_skin_binding(
    ufbx_skin_deformer *skin,
    const std::vector<ufbx_node*>& joint_order,
    SkeletonHandle skeleton)
{
    SkinBindingDescription description;
    description.skeleton = skeleton;
    description.inverse_bind_matrices.resize(joint_order.size());

    for (size_t i = 0; i < joint_order.size(); ++i)
    {
        ufbx_skin_cluster *skin_cluster = find_skin_cluster(skin->clusters, joint_order[i]);

        if (skin_cluster)
        {
            ufbx_matrix m = skin_cluster->geometry_to_bone;

            description.inverse_bind_matrices[i] =
                simd_matrix(
                    simd_make_float4(static_cast<float>(m.m00), static_cast<float>(m.m10), static_cast<float>(m.m20), 0.0f),
                    simd_make_float4(static_cast<float>(m.m01), static_cast<float>(m.m11), static_cast<float>(m.m21), 0.0f),
                    simd_make_float4(static_cast<float>(m.m02), static_cast<float>(m.m12), static_cast<float>(m.m22), 0.0f),
                    simd_make_float4(static_cast<float>(m.m03), static_cast<float>(m.m13), static_cast<float>(m.m23), 1.0f)
                );
        }
        else
        {
            description.inverse_bind_matrices[i] = matrix_identity_float4x4;
        }
    }

    return description;
}

static const ufbx_baked_node *find_baked_node(const ufbx_baked_anim *baked_anim, const ufbx_node *joint)
{
    for (size_t i = 0; i < baked_anim->nodes.count; ++i)
    {
        if (baked_anim->nodes[i].typed_id == joint->typed_id)
        {
            return &baked_anim->nodes[i];
        }
    }

    return nullptr;
}

AnimationClipDescription AnimationImporter::import_clip(
    ufbx_scene *scene,
    ufbx_anim_stack *stack,
    const std::vector<ufbx_node*>& joint_order,
    SkeletonHandle skeleton)
{
    ufbx_bake_opts opts = {};
    // same as sample track comment in AnimationSystem
    opts.resample_rate = 60.0;

    ufbx_error err;
    ufbx_baked_anim *baked_animation = ufbx_bake_anim(scene, stack->anim, &opts, &err);

    if (!baked_animation) return AnimationClipDescription{};
    
    AnimationClipDescription description;
    description.skeleton = skeleton;
    description.duration = static_cast<float>(baked_animation->playback_duration);
    description.joint_tracks.resize(joint_order.size());

    for (size_t i = 0; i < joint_order.size(); ++i)
    {
        const ufbx_baked_node *baked_node = find_baked_node(baked_animation, joint_order[i]);

        if (!baked_node) continue;

        size_t t_key_count = baked_node->translation_keys.count;
        size_t r_key_count = baked_node->rotation_keys.count;
        size_t s_key_count = baked_node->scale_keys.count;
        size_t max_key_count = std::max({ t_key_count, r_key_count, s_key_count });

        if (max_key_count == 0) continue;

        description.joint_tracks[i].keyframes.reserve(max_key_count);

        for (size_t k = 0; k < max_key_count; ++k)
        {
            BakedKeyframe key;

            if (k < t_key_count)
            {
                key.time = static_cast<float>(baked_node->translation_keys[k].time);
            }
            else if (k < r_key_count)
            {
                key.time = static_cast<float>(baked_node->rotation_keys[k].time);
            }
            else if (k < s_key_count)
            {
                key.time = static_cast<float>(baked_node->scale_keys[k].time);
            }

            size_t t_index;
            size_t r_index;
            size_t s_index;

            if (t_key_count > 0)
            {
                t_index = std::min(k, t_key_count - 1);
            }
            else
            {
                t_index = 0;
            }

            if (r_key_count > 0)
            {
                r_index = std::min(k, r_key_count - 1);
            }
            else
            {
                r_index = 0;
            }

            if (s_key_count > 0)
            {
                s_index = std::min(k, s_key_count - 1);
            }
            else
            {
                s_index = 0;
            }

            ufbx_vec3 t;
            ufbx_quat r;
            ufbx_vec3 s;

            if (t_key_count > 0)
            {
                t = baked_node->translation_keys[t_index].value;
            }
            else
            {
                t = joint_order[i]->local_transform.translation;
            }

            if (r_key_count > 0)
            {
                r = baked_node->rotation_keys[r_index].value;
            }
            else
            {
                r = joint_order[i]->local_transform.rotation;
            }

            if (s_key_count > 0)
            {
                s = baked_node->scale_keys[s_index].value;
            }
            else
            {
                s = joint_order[i]->local_transform.scale;
            }

            key.translation =
                simd_make_float3(
                    static_cast<float>(t.x),
                    static_cast<float>(t.y),
                    static_cast<float>(t.z));
    
            key.rotation =
                simd_quaternion(
                    static_cast<float>(r.x),
                    static_cast<float>(r.y),
                    static_cast<float>(r.z),
                    static_cast<float>(r.w));

            key.scale =
                simd_make_float3(
                    static_cast<float>(s.x),
                    static_cast<float>(s.y),
                    static_cast<float>(s.z));

            description.joint_tracks[i].keyframes.push_back(key);
        }
    }

    ufbx_free_baked_anim(baked_animation);

    return description;
}
