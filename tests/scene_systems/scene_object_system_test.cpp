#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "scene_systems/scene_object_system.hpp"

static const SceneObjectSystemConfig system_config = {
    .submesh_system = nullptr,
    .transformation_system = nullptr,
    // .animation_system = nullptr,
    .material_system = nullptr,
};

TEST_CASE("adding one scene object without draining the input buffer should not change the number of scene objects", "[scene object system][scene system][add]")
{
    SceneObjectSystem system(system_config);
    REQUIRE(0 == system.count());

    system.add((SceneObject){
        .submesh_handle = 0,
        .transformation_handle = 0,
        .material_handle = 0,
    });

    REQUIRE(0 == system.count());
}

TEST_CASE("adding one scene object and draining the input buffer should increment the number of scene objects by one", "[scene object system][scene system][add]")
{
    SceneObjectSystem system(system_config);
    REQUIRE(0 == system.count());

    system.add((SceneObject){
        .submesh_handle = 0,
        .transformation_handle = 0,
        .material_handle = 0,
    });

    system.drain_additions_input_buffer();

    REQUIRE(1 == system.count());

    system.add((SceneObject){
        .submesh_handle = 1,
        .transformation_handle = 1,
        .material_handle = 1,
    });

    system.drain_additions_input_buffer();

    REQUIRE(2 == system.count());
}