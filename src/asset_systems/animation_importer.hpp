/**
 * @file animation_importer.hpp
 * @brief 
 */

#pragma once
#include "core_systems/animation_types.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"

class AnimationImporter
{
public:
    static SkeletonDescription import_skeleton(ufbx_skin_deformer *skin);
    static SkinBindingDescription import_skin_binding(
        ufbx_skin_deformer *skin,
        const std::vector<ufbx_node*>& joint_order,
        SkeletonHandle skeleton);
    static AnimationClipDescription import_clip(
        ufbx_scene *scene,
        ufbx_anim_stack *stack,
        const std::vector<ufbx_node*>& joint_order,
        SkeletonHandle skeleton);

private:
    static void import_joint(
        SkeletonDescription &description,
        const std::vector<ufbx_node*>& joints,
        size_t joint_index);
    static std::vector<ufbx_node*> topologically_sort_joints(ufbx_skin_deformer *skin);
};

// I still need tests