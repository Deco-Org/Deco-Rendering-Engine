/**
 * @file transformation_system_test.cpp
 * @brief Unit tests for the transformation system
 */

#include <catch2/catch_test_macros.hpp>
#include "asset_systems/transformation_system.hpp"
#include "transformation_system_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("reserving n transformation handles should return an array of n handles", "[transformation][handle]")
{
    TransformationSystem system;
    std::vector<TransformationHandle> handles = system.reserveHandles(4);
    REQUIRE(4 == handles.size());
}

TEST_CASE("adding a transformation increases size", "[transformation][add]")
{
    TransformationSystem system;

    numberOfTransformationsShouldBe(system, 0);
    std::vector<TransformationHandle> handles = system.reserveHandles(1);
    Transformation transformation = {
        .position = originPosition,
        .rotation = zeroQuaternion,
        .scale = defaultScale
    };
    TransformationHandle parent = NO_TRANSFORMATION_PARENT;
    system.add(&transformation, &parent, handles.data(), 1);

    numberOfTransformationsShouldBe(system, 0);
    system.drainRenderThreadAdditionsInputBuffer();
    numberOfTransformationsShouldBe(system, 1);
}

TEST_CASE("adding multiple transformations increases size", "[transformation][add]")
{
    TransformationSystem system;

    numberOfTransformationsShouldBe(system, 0);
    // std::vector<TransformationEntry> entries = nUnparentedTransformationEntries(system, 3);
    std::vector<Transformation> transformations = nTransformations(3);
    std::vector<TransformationHandle> handles = system.reserveHandles(3);
    std::vector<TransformationHandle> parents(handles.size(), NO_TRANSFORMATION_PARENT);

    numberOfTransformationsShouldBe(system, 0);
    system.add(transformations.data(), parents.data(), handles.data(), 3);

    numberOfTransformationsShouldBe(system, 0);
    system.drainRenderThreadAdditionsInputBuffer();
    numberOfTransformationsShouldBe(system, 3);

    handlesAndIndicesShouldMatchUp(system);
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

    system.drainRenderThreadAdditionsInputBuffer();
    REQUIRE(simdFloat3Equal(somePosition, system.positions[system.handleToIndex[handle]]));
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

    system.drainRenderThreadAdditionsInputBuffer();
    REQUIRE(simdQuatfEqual(someRotation, system.rotations[system.handleToIndex[handle]]));
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

    system.drainRenderThreadAdditionsInputBuffer();
    REQUIRE(simdFloat3Equal(someScale, system.scales[system.handleToIndex[handle]]));
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

    system.drainRenderThreadAdditionsInputBuffer();

    TransformationHandle handle = system.add(
        (Transformation){
            .position = originPosition,
            .rotation = zeroQuaternion,
            .scale = someScale},
        parent);

    system.drainRenderThreadAdditionsInputBuffer();

    REQUIRE(parent == system.parentHandles[system.handleToIndex[handle]]);
}

TEST_CASE("adding transformations in bulk stores correct parents", "[transformation][add]")
{
    TransformationSystem system;
    std::vector<Transformation> transformations = nTransformations(2);
    std::vector<TransformationHandle> handles = system.reserveHandles(2);
    std::vector<TransformationHandle> parents = {NO_TRANSFORMATION_PARENT, handles[0]};

    system.add(transformations.data(), parents.data(), handles.data(), 2);
    system.drainRenderThreadAdditionsInputBuffer();
    numberOfTransformationsShouldBe(system, 2);
    REQUIRE(handles[0] == system.parentHandles[system.handleToIndex[handles[1]]]);
}

TEST_CASE("removing a transformation decreases size", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);
    numberOfTransformationsShouldBe(system, 3);

    TransformationHandle handleToRemove = system.indexToHandle[1];
    system.remove(&handleToRemove, 1);
    system.drainRenderThreadRemovalsInputBuffer();

    numberOfTransformationsShouldBe(system, 2);
    handlesAndIndicesShouldMatchUp(system);
}

TEST_CASE("removing multiple transformations decreases size", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(10);
    numberOfTransformationsShouldBe(system, 10);

    TransformationHandle handlesToRemove[3] = {
        system.indexToHandle[0],
        system.indexToHandle[5],
        system.indexToHandle[8]
    };

    system.remove(handlesToRemove, 3);
    system.drainRenderThreadRemovalsInputBuffer();

    numberOfTransformationsShouldBe(system, 7);
    handlesAndIndicesShouldMatchUp(system);
}

TEST_CASE("removing a transformation remaps handles to new indices", "[transformation][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);
    const simd_float3 *transformation0Position = &system.positions[system.handleToIndex[0]];
    const simd_float3 *transformation1Position = &system.positions[system.handleToIndex[1]];
    const simd_float3 *transformation2Position = &system.positions[system.handleToIndex[2]];

    TransformationHandle handleOfRemovedItem = system.indexToHandle[0];
    system.remove(&handleOfRemovedItem, 1);
    system.drainRenderThreadRemovalsInputBuffer();

    handlesAndIndicesShouldMatchUp(system);
}

TEST_CASE("removed transformations will have handles recycled", "[transformation][add][remove]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(3);

    TransformationHandle handleOfRemovedItem = 1;
    system.remove(&handleOfRemovedItem, 1);
    system.drainRenderThreadRemovalsInputBuffer();
    system.updateFreeHandles();

    std::vector<TransformationHandle> handles = system.reserveHandles(1);
    Transformation addedTransformation = {
        .position = somePosition,
        .rotation = someRotation,
        .scale = someScale
    };
    TransformationHandle parents = { NO_TRANSFORMATION_PARENT };
    system.add(
        &addedTransformation,
        &parents,
        handles.data(),
        1
    );

    system.drainRenderThreadAdditionsInputBuffer();

    REQUIRE((TransformationHandle){1} == handles[0]);

    TransformationHandle handle2 = someTransformationHandleForAddedTransformation(system);
    system.drainRenderThreadAdditionsInputBuffer();
    system.updateFreeHandles();

    REQUIRE((TransformationHandle){3} == handle2);
}

TEST_CASE("reparented transformations will remain after new parent when new parent comes prior to transformation", "[transformation][reparent]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(12);
    TransformationHandle parentHandle = 3;
    TransformationHandle childHandle = 5;
    uint32_t oldChildIndex = system.handleToIndex[childHandle];
    
    system.setParent(childHandle, parentHandle);
    uint32_t parentIndex = system.handleToIndex[parentHandle];
    uint32_t newChildIndex = system.handleToIndex[childHandle];
    
    REQUIRE(parentIndex < newChildIndex);
    REQUIRE(oldChildIndex == newChildIndex);
}

TEST_CASE("reparented transformations will be moved to be prior to new parent when new parent comes after transformation", "[transformation][reparent]")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(12);
    TransformationHandle parentHandle = 5;
    TransformationHandle childHandle = 3;
    uint32_t oldChildIndex = system.handleToIndex[childHandle];
    
    system.setParent(childHandle, parentHandle);
    uint32_t parentIndex = system.handleToIndex[parentHandle];
    uint32_t newChildIndex = system.handleToIndex[childHandle];
    
    REQUIRE(parentIndex < newChildIndex);
    handlesAndIndicesShouldMatchUp(system);
}

TEST_CASE("children of reparented transformation will remain subsequent to reparented transform")
{
    TransformationSystem system = makeTransformationSystemWithNTransformations(12);
    TransformationHandle parentHandle = 5;
    TransformationHandle childHandle = 3;
    TransformationHandle grandchildHandle = 4;
    uint32_t oldChildIndex = system.handleToIndex[childHandle];
    uint32_t oldGrandchildIndex = system.handleToIndex[grandchildHandle];
    
    system.setParent(childHandle, parentHandle);
    uint32_t parentIndex = system.handleToIndex[parentHandle];
    uint32_t newChildIndex = system.handleToIndex[childHandle];
    uint32_t newGrandchildIndex = system.handleToIndex[grandchildHandle];
    
    REQUIRE(parentIndex < newChildIndex);
    REQUIRE(newChildIndex < newGrandchildIndex);
    handlesAndIndicesShouldMatchUp(system);
}