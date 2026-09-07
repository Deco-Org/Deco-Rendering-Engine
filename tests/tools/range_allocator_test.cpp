#include <catch2/catch_test_macros.hpp>
#include "tools/range_allocator.hpp"

TEST_CASE("allocating more than capacity fails", "[tools][alloc]")
{
    RangeAllocator allocator(16);

    auto a = allocator.alloc(16);
    REQUIRE(a.has_value());
    
    auto b = allocator.alloc(1);
    REQUIRE_FALSE(b.has_value());
}

TEST_CASE("sequential allocations do not overlap", "[tools][alloc]")
{
    RangeAllocator allocator(16);
    auto a = allocator.alloc(4);
    auto b = allocator.alloc(6);

    REQUIRE(a->start == 0);
    REQUIRE(b->start >= a->start + a->size);
}

TEST_CASE("freeing and reallocating same size reuses the freed range", "[tools][alloc][free]")
{
    RangeAllocator allocator(16);
    auto a = allocator.alloc(10);

    allocator.free(*a);

    auto b = allocator.alloc(10);
    REQUIRE(b->start == a->start);
}

TEST_CASE("freeing two adjacent ranges coalesces them into one block", "[tools][alloc][free]")
{
    RangeAllocator allocator(16);
    auto a = allocator.alloc(7);
    auto b = allocator.alloc(7);

    allocator.free(*a);
    allocator.free(*b);

    auto c = allocator.alloc(14);
    REQUIRE(c.has_value());
    REQUIRE(c->start == 0);
}

TEST_CASE("freeing ranges out of order coalesces them correctly", "[tools][alloc][free]")
{
    RangeAllocator allocator(16);
    auto a = allocator.alloc(4);
    auto b = allocator.alloc(4);
    auto c = allocator.alloc(4);

    allocator.free(*b);
    allocator.free(*c);
    allocator.free(*a);

    auto d = allocator.alloc(12);
    REQUIRE(d.has_value());
    REQUIRE(d->start == 0);
}

TEST_CASE("alignment produces a start that is a multiple of the requested alignment", "[tools][alloc][alignment]")
{
    RangeAllocator allocator(16);
    auto a = allocator.alloc(3);
    REQUIRE(a.has_value());

    auto aligned = allocator.alloc(10, 4);
    REQUIRE(aligned->start % 4 == 0);
}

TEST_CASE("capacity reports the constructor value", "[tools][alloc][capacity]")
{
    RangeAllocator allocator(13);
    REQUIRE(allocator.capacity() == 13);
}