#include <catch2/catch_test_macros.hpp>
#include "tools/synchronized_buffer.hpp"

TEST_CASE("Buffer should be moved on std::move", "[tools][buffer][move]")
{
    SynchronizedBuffer<int> buffer1 = SynchronizedBuffer<int>(4);
    int items[4] = {32, 64, 128, 256};
    memcpy(buffer1.buffer, items, sizeof(items[0]) * 4);
    // Moving the buffer
    SynchronizedBuffer<int> buffer2 = std::move(buffer1);
    REQUIRE(0 == buffer1.count);
    REQUIRE(4 == buffer2.count);
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(items[i] == buffer2.buffer[i]);
    }
}

TEST_CASE("Buffer data should be moved on moveData()", "[tools][buffer][move]")
{
    SynchronizedBuffer<int> buffer = SynchronizedBuffer<int>(4);
    int items[4] = {32, 64, 128, 256};
    memcpy(buffer.buffer, items, sizeof(items[0]) * 4);
    // Moving the buffer data
    size_t movedDataSize = buffer.count;
    int* movedData = buffer.safely_extract_data();
    REQUIRE(0 == buffer.count);
    REQUIRE(4 == movedDataSize);
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(items[i] == movedData[i]);
    }
}