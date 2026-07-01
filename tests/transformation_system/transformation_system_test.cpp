/**
 * @file transformation_system_test.cpp
 * @brief Unit tests for the transformation system
 */

#include <catch2/catch_test_macros.hpp>
#include "asset_systems/transformation_system.hpp"
#include "transformation_system_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("adding a transformation increases size", "[transformation][add]")
{
    TransformationSystem system;

    REQUIRE(system.positions.size() == 0);
    REQUIRE(system.rotations.size() == 0);
    REQUIRE(system.scales.size() == 0);

    TransformationHandle handle = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = defaultScale},
        NO_TRANSFORMATION_PARENT);

    REQUIRE(system.positions.size() == 1);
    REQUIRE(system.rotations.size() == 1);
    REQUIRE(system.scales.size() == 1);
}

TEST_CASE("added transformation stores correct position", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation){
            .position = somePosition,
            .rotation = zeroQuaternion,
            .scale = defaultScale},
        NO_TRANSFORMATION_PARENT);

    REQUIRE(simdFloat3Equal(somePosition, system.positions[handle]));
}

TEST_CASE("added transformation stores correct rotation", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = someRotation,
            .scale = defaultScale},
        NO_TRANSFORMATION_PARENT);

    REQUIRE(simdQuatfEqual(someRotation, system.rotations[handle]));
}

TEST_CASE("added transformation stores correct scale", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale},
        NO_TRANSFORMATION_PARENT);

    REQUIRE(simdFloat3Equal(someScale, system.scales[handle]));
}

TEST_CASE("added transformation stores correct parent", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle parent = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale},
        NO_TRANSFORMATION_PARENT);

    TransformationHandle handle = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale},
        parent);

    REQUIRE(parent == system.parentIndices[handle]);
}

TEST_CASE("removing a transformation decreases size", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);
    REQUIRE(system.positions.size() == 3);
    REQUIRE(system.rotations.size() == 3);
    REQUIRE(system.scales.size() == 3);

    system.remove((TransformationHandle){2});

    REQUIRE(system.positions.size() == 2);
    REQUIRE(system.rotations.size() == 2);
    REQUIRE(system.scales.size() == 2);
}

TEST_CASE("removing a transformation remaps handles to new indices", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);
    const simd_float3 *transformation0Position = &system.positions[system.handleToIndex[0]];
    const simd_float3 *transformation1Position = &system.positions[system.handleToIndex[1]];
    const simd_float3 *transformation2Position = &system.positions[system.handleToIndex[2]];

    system.remove((TransformationHandle){1});

    // Making sure indices and handles still line up
    REQUIRE(transformation0Position == &system.positions[system.handleToIndex[0]]);
    REQUIRE(transformation1Position == &system.positions[system.handleToIndex[2]]);
}

TEST_CASE("removed transformations will have handles recycled", "[transformation][add][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);

    system.remove((TransformationHandle){1});

    TransformationHandle handle1 = system.add(
        (Transformation){
            .position = somePosition,
            .rotation = someRotation,
            .scale = someScale},
        NO_TRANSFORMATION_PARENT);

    REQUIRE(handle1 == (TransformationHandle){1});

    TransformationHandle handle2 = someTransformationHandleForAddedTransformation(system);

    REQUIRE(handle2 == (TransformationHandle){3});
}