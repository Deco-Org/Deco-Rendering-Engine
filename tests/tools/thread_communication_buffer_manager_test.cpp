#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "tools/thread_communication_buffer_manager.hpp"

using SomeType = uint8_t;
using SomeHandle = size_t;

struct SomeEntryType
{
    SomeType item;
    SomeHandle handle;
};

TEST_CASE("adding an item to a custom buffer should result in the data being copied into the buffer", "[thread communication][add][write]")
{
    SynchronizedBuffer<SomeType> custom_buffer;
    SomeType some_data[4] = {0, 1, 2, 3};
    SomeType some_other_data[4] = {4, 5, 6, 7};

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4);

    REQUIRE(4 == custom_buffer.count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i] == custom_buffer.buffer[i]);
    }

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_other_data,
        4);
    
    REQUIRE(8 == custom_buffer.count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i] == custom_buffer.buffer[i]);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_other_data[i] == custom_buffer.buffer[4 + i]);
    }
}

TEST_CASE("adding an item and a max handle to a custom buffer should result in the data and the max handle being copied into the buffer", "[thread communication][add][write]")
{
    SystemInputBuffer<SomeEntryType, SomeHandle> custom_buffer;
    SomeEntryType some_data[4] = {
        {.item = 1, .handle = 0},
        {.item = 2, .handle = 1},
        {.item = 3, .handle = 2},
        {.item = 4, .handle = 3},
    };
    SomeEntryType some_other_data[4] = {
        {.item = 5, .handle = 4},
        {.item = 6, .handle = 5},
        {.item = 7, .handle = 6},
        {.item = 8, .handle = 7},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4,
        0);

    REQUIRE(4 == custom_buffer.count);
    REQUIRE(0 == custom_buffer.max_handle);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i].item == custom_buffer.buffer[i].item);
        REQUIRE(some_data[i].handle == custom_buffer.buffer[i].handle);
    }

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_other_data,
        4,
        3);
    
    REQUIRE(8 == custom_buffer.count);
    REQUIRE(3 == custom_buffer.max_handle);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i].item == custom_buffer.buffer[i].item);
        REQUIRE(some_data[i].handle == custom_buffer.buffer[i].handle);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_other_data[i].item == custom_buffer.buffer[4 + i].item);
        REQUIRE(some_other_data[i].handle == custom_buffer.buffer[4 + i].handle);
    }
}

TEST_CASE("adding an item and a max handle to the additions buffer should result in the data and the max handle being copied into the buffer", "[thread communication][additions buffer][add][write]")
{
    SomeEntryType some_data[4] = {
        {.item = 9, .handle = 0},
        {.item = 3, .handle = 1},
        {.item = 6, .handle = 2},
        {.item = 1, .handle = 3},
    };
    SomeEntryType some_other_data[4] = {
        {.item = 8, .handle = 4},
        {.item = 6, .handle = 5},
        {.item = 7, .handle = 6},
        {.item = 8, .handle = 7},
    };

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_input.count);

    manager.add_to_additions_buffer(some_data, 4, 0);

    REQUIRE(4 == manager.additions_input.count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i].item == manager.additions_input.buffer[i].item);
        REQUIRE(some_data[i].handle == manager.additions_input.buffer[i].handle);
    }

    manager.add_to_additions_buffer(some_other_data, 4, 3);
    for (size_t i = 0; i < 4; ++i)
    {
            REQUIRE(some_data[i].item == manager.additions_input.buffer[i].item);
            REQUIRE(some_data[i].handle == manager.additions_input.buffer[i].handle);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_other_data[i].item == manager.additions_input.buffer[i + 4].item);
        REQUIRE(some_other_data[i].handle == manager.additions_input.buffer[i + 4].handle);
    }
}