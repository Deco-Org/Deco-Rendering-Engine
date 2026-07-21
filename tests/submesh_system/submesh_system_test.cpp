/**
 * @file submesh_system_test.cpp
 * @brief Unit tests for the submesh system
 */

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "asset_systems/submesh_system.hpp"
#include "submesh_system_test_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("adding one submesh and draining the input buffer should add a new vertex buffer and a new index buffer to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == system.vertexBuffers.size());

    ufbx_scene* scene = aSceneWithACubeModel();
    
    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];
    system.add(cubeMesh, &submesh);
    REQUIRE(0 == system.vertexBuffers.size());
    system.drainAdditionsInputBuffer();

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

    ufbx_scene* scene = aSceneWithACubeModel();

    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];
    system.add(cubeMesh, &submesh);
    system.drainAdditionsInputBuffer();

    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(1 == consumedHandles.size());

    aSceneIsFreed(scene);
}

TEST_CASE("adding n submeshes and draining the input buffer should add n entries to the system", "[submesh][asset system][add]")
{
    SubmeshSystem system;
    REQUIRE(0 == numberOfSubmeshesInSystem(system));

    ufbx_scene* scene = aSceneWithACubeModel();

    ufbx_mesh* cubeMesh = scene->meshes.data[0];
    ufbx_mesh_part submesh = cubeMesh->material_parts.data[0];

    system.add(cubeMesh, &submesh);
    system.add(cubeMesh, &submesh);
    system.drainAdditionsInputBuffer();

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
    system.drainAdditionsInputBuffer();
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
    system.drainAdditionsInputBuffer();
    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    REQUIRE(mesh->material_parts.count == consumedHandles.size());
    size_t numberOfFreeHandles = system.freeHandles.size();
    SubmeshHandle removedHandle = consumedHandles[0];

    // When an item is removed
    system.remove((SubmeshList) {
        .data = &removedHandle,
        .count = 1
    });

    system.drainRemovalBuffer();
    
    REQUIRE(numberOfFreeHandles + 1 == system.freeHandles.size());
    REQUIRE(removedHandle == system.freeHandles[system.freeHandles.size() - 1]);

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
    system.drainAdditionsInputBuffer();
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

    aSceneIsFreed(scene);
}

TEST_CASE("removed submeshes should have their handles recycled", "[submesh][asset system][add][remove]")
{
    SubmeshSystem system;

    ufbx_scene* lampScene = aSceneWithATestLampModel();
    ufbx_mesh* lampMesh = lampScene->meshes.data[0];

    system.add(lampMesh);
    system.drainAdditionsInputBuffer();
    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();
    SubmeshHandle largestHandle = system.largestHandle;

    SubmeshHandle removedHandle = consumedHandles[1];

    system.remove((SubmeshList) {
        .data = &removedHandle,
        .count = 1
    });
    system.drainRemovalBuffer();
    size_t oldNumberOfFreeHandles = system.freeHandles.size();
    REQUIRE(1 == system.freeHandles.size());
    REQUIRE(largestHandle == system.largestHandle);

    ufbx_scene* cubeScene = aSceneWithACubeModel();
    ufbx_mesh* cubeMesh = cubeScene->meshes.data[0];
    SubmeshHandle additionHandle = system.add(cubeMesh)[0];
    system.drainAdditionsInputBuffer();

    REQUIRE(removedHandle == additionHandle);
    REQUIRE(oldNumberOfFreeHandles - 1 == system.freeHandles.size());
    REQUIRE(largestHandle == system.largestHandle);

    aSceneIsFreed(lampScene);
    aSceneIsFreed(cubeScene);
}

TEST_CASE("submeshes should only be added and removed after the the additions and removal input buffers are drained", "[submesh][asset system][add][remove]")
{
    SubmeshSystem system;
    ufbx_scene* lampScene = aSceneWithATestLampModel();
    ufbx_mesh* lampMesh = lampScene->meshes.data[0];

    system.add(lampMesh);
    REQUIRE(0 == numberOfSubmeshesInSystem(system));
    system.drainAdditionsInputBuffer();
    size_t numberOfSubmeshesPriorToRemoval = numberOfSubmeshesInSystem(system);
    REQUIRE(lampMesh->material_parts.count == numberOfSubmeshesPriorToRemoval);

    std::vector<SubmeshHandle> consumedHandles = system.getItemsAndDrainOutputBuffer();

    system.remove((SubmeshList) {
        .data = &consumedHandles[0],
        .count = 1
    });
    REQUIRE(lampMesh->material_parts.count == numberOfSubmeshesPriorToRemoval);
    system.drainRemovalBuffer();
    REQUIRE(lampMesh->material_parts.count - 1 == numberOfSubmeshesInSystem(system));

    aSceneIsFreed(lampScene);
}


TEST_CASE("submeshes should be able to be added to and removed from the system while the render thread loops over the items in the system", "[submesh][asset system][add][remove][threading]")
{
    SubmeshSystem system;
    std::atomic<bool> loadingThreadDone = false;
    size_t expectedNumberOfLivingHandles = 0;
    
    {
         std::jthread renderThread([&system, &loadingThreadDone]() {
            while (!loadingThreadDone)
            {
                system.drainRemovalBuffer();
                system.drainAdditionsInputBuffer();
            }
        });

        std::jthread loadingThread([&system, &loadingThreadDone, &expectedNumberOfLivingHandles]() {
            ufbx_scene* lampScene = aSceneWithATestLampModel();

            std::vector<std::vector<SubmeshHandle>> meshes;

            for (ufbx_mesh* mesh : lampScene->meshes)
            {
                meshes.push_back(system.add(mesh));
                expectedNumberOfLivingHandles += 1;
            }

            for (std::vector<SubmeshHandle>& submeshes : meshes)
            {
                system.remove((SubmeshList) {
                    .data = submeshes.data(),
                    .count = submeshes.size()
                });
                expectedNumberOfLivingHandles -= submeshes.size();
                std::vector<SubmeshHandle> newlyFreedHandles = system.getItemsAndDrainOutputBuffer();
            }

            ufbx_scene* cubeScene = aSceneWithACubeModel();
            for (ufbx_mesh* mesh : cubeScene->meshes)
            {
                meshes.push_back(system.add(mesh));
                expectedNumberOfLivingHandles += 1;
            }

            loadingThreadDone = true;
            aSceneIsFreed(lampScene);
            aSceneIsFreed(cubeScene);
        });

        REQUIRE(expectedNumberOfLivingHandles == system.indexCounts.size() - system.freeHandles.size());

        allFreeHandlesShouldBeTombstones(system);
        allTombstonesShouldBeFreeHandles(system);
    }
}