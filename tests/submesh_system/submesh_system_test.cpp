/**
 * @file submesh_system_test.cpp
 * @brief Unit tests for the submesh system
 */

#pragma once

#include <catch2/catch_test_macros.hpp>
#include "asset_systems/submesh_system.hpp"
#include "test_utils.hpp"

TEST_CASE("adding one submesh and draining the input buffer should add a new vertex buffer and a new index buffer to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == system.vertexBuffers.size());

    // ufbx_mesh_part meshPart = {
    //     .index = 0,
    //     .num_faces = 1,
    //     .num_triangles = 2,
    //     .num_empty_faces = 0,
    //     .num_point_faces = 0,
    //     .num_line_faces = 0,
    // }
}