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