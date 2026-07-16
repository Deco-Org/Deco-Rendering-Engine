/**
 * @file submesh_system_test.cpp
 * @brief Unit tests for the submesh system
 */

#pragma once

#include <catch2/catch_test_macros.hpp>
#include "asset_systems/submesh_system.hpp"
#include "submesh_system_test_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("adding one submesh and draining the input buffer should add a new vertex buffer and a new index buffer to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == system.vertexBuffers.size());

    ufbx_scene* scene = aCubeHasBeenLoadedIntoAScene();
    
    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];
    system.add(cubeMesh, &submesh);
    REQUIRE(0 == system.vertexBuffers.size());
    system.drainInputBuffer();

    REQUIRE(1 == system.vertexBuffers.size());
    REQUIRE(1 == system.indexBuffers.size());

    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(1 == consumedHandles.size());

    aSceneIsFreed(scene);
}

TEST_CASE("the output buffer should contain a single submesh if the input buffer was drained with a single submesh inside", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == system.vertexBuffers.size());

    ufbx_scene* scene = aCubeHasBeenLoadedIntoAScene();

    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];
    system.add(cubeMesh, &submesh);
    system.drainInputBuffer();

    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(1 == consumedHandles.size());

    aSceneIsFreed(scene);
}

TEST_CASE("adding n submeshes and draining the input buffer should add n entries to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == numberOfSubmeshesInSystem(system));

    ufbx_scene* scene = aCubeHasBeenLoadedIntoAScene();

    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];

    system.add(cubeMesh, &submesh);
    system.add(cubeMesh, &submesh);
    system.drainInputBuffer();

    REQUIRE(2 == system.vertexBuffers.size());

    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(2 == consumedHandles.size());
    REQUIRE(consumedHandles[0] != consumedHandles[1]);

    aSceneIsFreed(scene);
}

TEST_CASE("adding n submeshes that are a part of a shared parent mesh should add n entries to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == numberOfSubmeshesInSystem(system));
    REQUIRE(INVALID_SUBMESH_HANDLE == system.largestHandle);

    ufbx_scene* scene = aSceneWithATestLampModel();

    ufbx_mesh* mesh = scene->meshes.data[0];
    system.add(mesh);
    system.drainInputBuffer();
    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(mesh->material_parts.count == consumedHandles.size());

    aSceneIsFreed(scene);
}

TEST_CASE("removing a submesh from the system should free the handle", "[submesh][asset system][remove]")
{
    SubmeshSystem system;
    REQUIRE(0 == numberOfSubmeshesInSystem(system));
    REQUIRE(INVALID_SUBMESH_HANDLE == system.largestHandle);

    ufbx_scene* scene = aSceneWithATestLampModel();
    ufbx_mesh* mesh = scene->meshes.data[0];

    system.add(mesh);
    system.drainInputBuffer();
    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(mesh->material_parts.count == consumedHandles.size());
    size_t numberOfFreeHandles = system.freeHandles.size();

    // When an item is removed
    system.remove((SubmeshList) {
        .data = &consumedHandles[0],
        .count = 1
    });

    system.drainRemovalBuffer();
    
    REQUIRE(numberOfFreeHandles + 1 == system.freeHandles.size());

    aSceneIsFreed(scene);
}

TEST_CASE("removing n submeshes from the system should free n handles", "[submesh][asset system][remove]")
{
    SubmeshSystem system;
    REQUIRE(0 == numberOfSubmeshesInSystem(system));
    REQUIRE(INVALID_SUBMESH_HANDLE == system.largestHandle);

    ufbx_scene* scene = aSceneWithATestLampModel();
    ufbx_mesh* mesh = scene->meshes.data[0];

    system.add(mesh);
    system.drainInputBuffer();
    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(mesh->material_parts.count == consumedHandles.size());
    size_t numberOfFreeHandles = system.freeHandles.size();

    // When an item is removed
    system.remove((SubmeshList) {
        .data = consumedHandles.data(),
        .count = consumedHandles.size()
    });

    system.drainRemovalBuffer();

    REQUIRE(numberOfFreeHandles + consumedHandles.size() == system.freeHandles.size());
    // consumedHandles and freeHandles should have the same data
    REQUIRE(0 == memcmp(consumedHandles.data(), system.freeHandles.data(), consumedHandles.size() * sizeof(SubmeshHandle)));
}