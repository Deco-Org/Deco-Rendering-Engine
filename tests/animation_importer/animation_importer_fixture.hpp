/**
 * @file animation_importer_fixture.hpp
 * @brief
 */

#pragma once
#include "asset_systems/animation_importer.hpp"
#include <algorithm>

inline size_t find_joint_index(const std::vector<ufbx_node*>& joints, ufbx_node *target)
{
    auto it = std::find(joints.begin(), joints.end(), target);
    if (it != joints.end())
    {
        return std::distance(joints.begin(), it);
    }

    return static_cast<size_t>(-1);
}