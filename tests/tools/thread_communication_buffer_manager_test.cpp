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

TEST_CASE("adding an item to a custom buffer should result in the data being copied into the buffer", "[thread communication buffer manager][add]")
{
    SynchronizedBuffer<SomeType> custom_buffer;
    SomeType some_data[4] = {0, 1, 2, 3};
    SomeType some_other_data[4] = {4, 5, 6, 7};

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunicationBufferManager<SomeType, SomeHandle>::add_to_buffer(
        custom_buffer,
        some_data,
        4);

    REQUIRE(4 == custom_buffer.count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i] == custom_buffer.buffer[i]);
    }

    ThreadCommunicationBufferManager<SomeType, SomeHandle>::add_to_buffer(
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

TEST_CASE("adding an item and a max handle to a custom buffer should result in the data and the max handle being copied into the buffer", "[thread communication buffer manager][add]")
{
    SystemInputBuffer<SomeEntryType, SomeHandle> custom_buffer;
    SomeEntryType some_data[4] = {
        (SomeEntryType){.item = 1, .handle = 0},
        (SomeEntryType){.item = 2, .handle = 1},
        (SomeEntryType){.item = 3, .handle = 2},
        (SomeEntryType){.item = 4, .handle = 3},
    };
    SomeEntryType some_other_data[4] = {
        (SomeEntryType){.item = 5, .handle = 4},
        (SomeEntryType){.item = 6, .handle = 5},
        (SomeEntryType){.item = 7, .handle = 6},
        (SomeEntryType){.item = 8, .handle = 7},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunicationBufferManager<SomeEntryType, SomeHandle>::add_to_buffer(
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

    ThreadCommunicationBufferManager<SomeEntryType, SomeHandle>::add_to_buffer(
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