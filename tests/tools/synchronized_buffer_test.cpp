#include <catch2/catch_test_macros.hpp>
#include "tools/synchronized_buffer.hpp"

TEST_CASE("Buffer should preserve order of added items", "[add][tools][buffer]")
{
    SynchronizedBuffer<int> buffer = SynchronizedBuffer<int>(4);
    int items[4] = {32, 64, 128, 256};
    for (int i = 0; i < 4; ++i)
    {
        buffer.set(i, items[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(items[i] == buffer.at(i));
    }
}

TEST_CASE("Buffer should be moved on std::move", "[tools][buffer][move]")
{
    SynchronizedBuffer<int> buffer1 = SynchronizedBuffer<int>(4);
    int items[4] = {32, 64, 128, 256};
    for (int i = 0; i < 4; ++i)
    {
        buffer1.set(i, items[i]);
    }
    // Moving the buffer
    SynchronizedBuffer<int> buffer2 = std::move(buffer1);
    REQUIRE(0 == buffer1.size());
    REQUIRE(4 == buffer2.size());
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(items[i] == buffer2.at(i));
    }
}

TEST_CASE("Buffer data should be moved on moveData()", "[tools][buffer][move]")
{
    SynchronizedBuffer<int> buffer = SynchronizedBuffer<int>(4);
    int items[4] = {32, 64, 128, 256};
    for (int i = 0; i < 4; ++i)
    {
        buffer.set(i, items[i]);
    }
    // Moving the buffer data
    size_t movedDataSize = buffer.size();
    int* movedData = buffer.moveData();
    REQUIRE(0 == buffer.size());
    REQUIRE(4 == movedDataSize);
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(items[i] == movedData[i]);
    }
}