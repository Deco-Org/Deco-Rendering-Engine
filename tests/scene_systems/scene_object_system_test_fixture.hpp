/**
 * @file scene_object_system_test_fixture.hpp
 * @brief
 */

#pragma once
#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "scene_systems/scene_object_system.hpp"

constexpr uint64_t get_final_n_bits(uint64_t value, SortKeyBitMappings::Widths n)
{
    return value & ((static_cast<uint64_t>(1U) << static_cast<uint64_t>(n)) - 1);
}

static ufbx_scene* a_scene_with_a_test_lamp_model()
{
    const char filepath[24] = "assets/test_lamp_01.fbx";
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(filepath, &opts, &error);
    REQUIRE(scene);
    return scene;
}

void a_scene_is_freed(ufbx_scene* scene)
{
    ufbx_free_scene(scene);
}