/**
 * @file transformation_system_test.cpp
 * @brief Unit tests for the transformation system
 */

#pragma once
#include <catch2/catch_test_macros.hpp>
#include "asset_systems/transformation_system.hpp"
#include "transformation_system_fixture.hpp"
#include "test_utils.hpp"

constexpr simd_float3 originPosition = { 0.0f, 0.0f, 0.0f };
constexpr simd_quatf zeroQuaternion = { { 0.0f, 0.0f, 0.0f, 0.0f } };
constexpr simd_float3 defaultScale = { 1.0f, 1.0f, 1.0f };

constexpr simd_float3 somePosition = { 1.0f, 3.0f, 12.0f };
constexpr simd_quatf someRotation = { { 0.3f, 0.5f, 0.9f, 0.8f } };
constexpr simd_float3 someScale = { 0.5f, 2.0f, 3.6f };

TEST_CASE("adding a transformation increases size", "[transformation][add]")
{
    TransformationSystem system;

    REQUIRE(system.positions.size() == 0);
    REQUIRE(system.rotations.size() == 0);
    REQUIRE(system.scales.size() == 0);

    TransformationHandle handle = system.add(
        (Transformation) {
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = defaultScale
        },
        NO_TRANSFORMATION_PARENT
    );

    REQUIRE(system.positions.size() == 1);
    REQUIRE(system.rotations.size() == 1);
    REQUIRE(system.scales.size() == 1);
}

TEST_CASE("added transformation stores correct position", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation) {
            .position = somePosition,
            .rotation = zeroQuaternion,
            .scale = defaultScale
        },
        NO_TRANSFORMATION_PARENT
    );

    REQUIRE(simdFloat3Equal(somePosition, system.positions[handle]));
}

TEST_CASE("added transformation stores correct rotation", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation) {
            .position = originPosition,
            .rotation = someRotation,
            .scale = defaultScale
        },
        NO_TRANSFORMATION_PARENT
    );

    REQUIRE(simdQuatfEqual(someRotation, system.rotations[handle]));
}

TEST_CASE("added transformation stores correct scale", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle handle = system.add(
        (Transformation) {
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale
        },
        NO_TRANSFORMATION_PARENT
    );

    REQUIRE(simdFloat3Equal(someScale, system.scales[handle]));
}

TEST_CASE("added transformation stores correct parent", "[transformation][add]")
{
    TransformationSystem system;

    TransformationHandle parent = system.add(
        (Transformation) {
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale
        },
        NO_TRANSFORMATION_PARENT
    );

    TransformationHandle handle = system.add(
        (Transformation) {
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale
        },
        parent
    );

    REQUIRE(parent == system.parentIndices[handle]);
}

TEST_CASE("removing a transformation decreases size", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);
    REQUIRE(system.positions.size() == 3);
    REQUIRE(system.rotations.size() == 3);
    REQUIRE(system.scales.size() == 3);

    system.remove((TransformationHandle) { 2 });

    REQUIRE(system.positions.size() == 2);
    REQUIRE(system.rotations.size() == 2);
    REQUIRE(system.scales.size() == 2);
}