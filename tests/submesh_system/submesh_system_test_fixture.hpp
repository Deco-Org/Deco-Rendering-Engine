/**
 * @file submesh_system_test_fixture.hpp
 */
#pragma once
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include "asset_systems/submesh_system.hpp"

ufbx_scene* aSceneWithACubeModel()
{
    const char cubeFilepath[21] = "assets/test_cube.fbx";
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(cubeFilepath, &opts, &error);
    REQUIRE(scene);
    return scene;
}

ufbx_scene* aSceneWithATestLampModel()
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

inline size_t numberOfSubmeshesInSystem(SubmeshSystem& system)
{
    return system.indexBuffers.size() - system.freeHandles.size();
}

inline void aSceneIsFreed(ufbx_scene* scene)
{
    ufbx_free_scene(scene);
}

void allFreeHandlesShouldBeTombstones(SubmeshSystem& system)
{
    for (SubmeshHandle handle : system.freeHandles)
    {
        REQUIRE(system.isTombstone(handle));
    }
}

void allTombstonesShouldBeFreeHandles(SubmeshSystem& system)
{
    for (SubmeshHandle handle = 0; handle < system.indexBuffers.size(); ++handle)
    {
        if (system.indexCounts[handle] == INVALID_INDEX_COUNT)
            REQUIRE(std::find(system.freeHandles.begin(), system.freeHandles.end(), handle) != system.freeHandles.end());
    }
}