/**
 * @file animation_importer.cpp
 * @brief 
 */

#include "animation_importer.hpp"

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

std::vector<ufbx_node*> AnimationImporter::import_joint(
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
}

AnimationClipDescription AnimationImporter::import_clip(
    ufbx_scene *scene,
    ufbx_anim_stack *stack,
    const std::vector<ufbx_node*>& joint_order,
    SkeletonHandle skeleton)
{
    
}
