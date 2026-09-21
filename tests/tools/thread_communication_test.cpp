#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "tools/thread_communication.hpp"

using SomeType = uint8_t;
using SomeHandle = size_t;

struct SomeEntryType
{
    SomeType item;
    SomeHandle handle;
};

TEST_CASE("adding an item to a custom buffer should result in the data being copied into the buffer", "[thread communication][custom buffer][add][write]")
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

TEST_CASE("adding an item and a max handle to a custom buffer should result in the data and the max handle being copied into the buffer", "[thread communication][custom buffer][add][write]")
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

    manager.add_to_additions_input_buffer(some_data, 4, 0);

    REQUIRE(4 == manager.additions_input.count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i].item == manager.additions_input.buffer[i].item);
        REQUIRE(some_data[i].handle == manager.additions_input.buffer[i].handle);
    }

    manager.add_to_additions_input_buffer(some_other_data, 4, 3);
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

TEST_CASE("calling the method to add an item to the removals buffer should copy the specified handle to the removals buffer", "[thread communication][removals buffer][add][write]")
{
    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.removals_input.count);

    SomeHandle some_handle_to_remove = 67;
    SomeHandle some_other_handle_to_remove = 21;

    manager.add_to_removals_input_buffer(&some_handle_to_remove, 1);

    REQUIRE(1 == manager.removals_input.count);
    REQUIRE(some_handle_to_remove == manager.removals_input.buffer[0]);

    manager.add_to_removals_input_buffer(&some_other_handle_to_remove, 1);

    REQUIRE(2 == manager.removals_input.count);
    REQUIRE(some_handle_to_remove == manager.removals_input.buffer[0]);
    REQUIRE(some_other_handle_to_remove == manager.removals_input.buffer[1]);
}

TEST_CASE("calling the method to add an item to the additions output buffer should copy the item to the additions output buffer", "[thread communication][additions output buffer][add][write]")
{
    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_output.count);

    SomeHandle some_handle_to_be_outputted = 67;
    SomeHandle some_other_handle_to_be_outputted = 21;
    SomeHandle some_max_handle = 2;
    SomeHandle some_other_max_handle = 3;

    manager.add_to_additions_output_buffer(&some_handle_to_be_outputted, 1, some_max_handle);

    REQUIRE(1 == manager.additions_output.count);
    REQUIRE(some_handle_to_be_outputted == manager.additions_output.buffer[0]);
    REQUIRE(some_max_handle == manager.additions_output.max_handle);

    manager.add_to_additions_output_buffer(&some_other_handle_to_be_outputted, 1, some_other_max_handle);
    REQUIRE(some_other_max_handle == manager.additions_output.max_handle);

    REQUIRE(2 == manager.additions_output.count);
    REQUIRE(some_handle_to_be_outputted == manager.additions_output.buffer[0]);
    REQUIRE(some_other_handle_to_be_outputted == manager.additions_output.buffer[1]);
}

TEST_CASE("draining a custom buffer with a max handle and with nullptr as a specified output should clear the custom buffer", "[thread communication][custom buffer][drain][write]")
{
    SystemInputBuffer<SomeEntryType, SomeHandle> custom_buffer;
    SomeEntryType some_data[4] = {
        {.item = 1, .handle = 0},
        {.item = 2, .handle = 1},
        {.item = 3, .handle = 2},
        {.item = 4, .handle = 3},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4,
        3);

    REQUIRE(4 == custom_buffer.count);

    SECTION("having nullptr as the count output should clear the custom buffer")
    {
        SomeHandle max_handle = ThreadCommunication::drain_buffer_and_get_max_handle<SomeEntryType, SomeHandle>(custom_buffer, nullptr, nullptr);
        REQUIRE(3 == max_handle);
        REQUIRE(0 == custom_buffer.count);
    }

    SECTION("having a value as the count output should clear the custom buffer and output the count")
    {
        size_t count;
        size_t pre_drainage_count = custom_buffer.count;
        SomeHandle max_handle = ThreadCommunication::drain_buffer_and_get_max_handle<SomeEntryType, SomeHandle>(custom_buffer, nullptr, &count);
        REQUIRE(3 == max_handle);
        REQUIRE(0 == custom_buffer.count);
        REQUIRE(pre_drainage_count == count);
    }
}

TEST_CASE("draining a custom buffer with a max handle should drain the items into a specified output", "[thread communication][custom buffer][drain][read][write]")
{
    SystemInputBuffer<SomeEntryType, SomeHandle> custom_buffer;
    SomeEntryType some_data[4] = {
        {.item = 1, .handle = 0},
        {.item = 2, .handle = 1},
        {.item = 3, .handle = 2},
        {.item = 4, .handle = 3},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4,
        3);

    REQUIRE(4 == custom_buffer.count);

    SomeEntryType* entries_output;
    size_t number_of_entries;
    SomeHandle max_handle = ThreadCommunication::drain_buffer_and_get_max_handle<SomeEntryType, SomeHandle>(custom_buffer, &entries_output, &number_of_entries);
    
    CHECK(3 == max_handle);
    REQUIRE(4 == number_of_entries);
    for (size_t i = 0; i < number_of_entries; ++i)
    {
        SomeEntryType* entry = entries_output + i;
        CAPTURE(i, entry);
        REQUIRE(some_data[i].item == entry->item);
        REQUIRE(some_data[i].handle == entry->handle);
    }
    REQUIRE(0 == custom_buffer.count);

    delete[] entries_output;
}

TEST_CASE("draining a custom buffer without a max handle and with nullptr as a specified output should clear the custom buffer", "[thread communication][custom buffer][drain][write]")
{
    SynchronizedBuffer<SomeEntryType> custom_buffer;
    SomeEntryType some_data[4] = {
        {.item = 1, .handle = 0},
        {.item = 2, .handle = 1},
        {.item = 3, .handle = 2},
        {.item = 4, .handle = 3},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4);

    REQUIRE(4 == custom_buffer.count);

    SECTION("having nullptr as the count output should clear the custom buffer")
    {
        ThreadCommunication::drain_buffer<SomeEntryType>(custom_buffer, nullptr, nullptr);
        REQUIRE(0 == custom_buffer.count);
    }

    SECTION("having a value as the count output should clear the custom buffer and output the count")
    {
        size_t count;
        size_t pre_drainage_count = custom_buffer.count;
        ThreadCommunication::drain_buffer<SomeEntryType>(custom_buffer, nullptr, &count);
        REQUIRE(0 == custom_buffer.count);
        REQUIRE(pre_drainage_count == count);
    }
}

TEST_CASE("draining a custom buffer without a max handle should drain the items into a specified output", "[thread communication][custom buffer][drain][read][write]")
{
    SynchronizedBuffer<SomeEntryType> custom_buffer;
    SomeEntryType some_data[4] = {
        {.item = 1, .handle = 0},
        {.item = 2, .handle = 1},
        {.item = 3, .handle = 2},
        {.item = 4, .handle = 3},
    };

    REQUIRE(0 == custom_buffer.count);

    ThreadCommunication::add_to_buffer(
        custom_buffer,
        some_data,
        4);

    REQUIRE(4 == custom_buffer.count);

    SomeEntryType* entries_output;
    size_t number_of_entries;
    ThreadCommunication::drain_buffer<SomeEntryType>(custom_buffer, &entries_output, &number_of_entries);
    
    REQUIRE(4 == number_of_entries);
    for (size_t i = 0; i < number_of_entries; ++i)
    {
        SomeEntryType* entry = entries_output + i;
        CAPTURE(i, entry);
        REQUIRE(some_data[i].item == entry->item);
        REQUIRE(some_data[i].handle == entry->handle);
    }
    REQUIRE(0 == custom_buffer.count);

    delete[] entries_output;
}

TEST_CASE("draining the additions input buffer with nullptr as a specified output should clear the additions input buffer", "[thread communication][additions input buffer][drain][write]")
{
    SomeEntryType some_data[4] = {
        {.item = 9, .handle = 0},
        {.item = 3, .handle = 1},
        {.item = 6, .handle = 2},
        {.item = 1, .handle = 3},
    };
    SomeHandle some_inputted_max_handle = 3;

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_input.count);

    manager.add_to_additions_input_buffer(some_data, 1, some_inputted_max_handle);
    REQUIRE(1 == manager.additions_input.count);

    SomeHandle max_handle;

    SECTION("having nullptr as the count output should clear the buffer")
    {
        manager.drain_additions_input_buffer(nullptr, nullptr, &max_handle);
        REQUIRE(some_inputted_max_handle == max_handle);
        REQUIRE(0 == manager.additions_input.count);
    }

    SECTION("having nullptr as the max handle output should clear the buffer")
    {
        manager.drain_additions_input_buffer(nullptr, nullptr, nullptr);
        REQUIRE(0 == manager.additions_input.count);
    }

    SECTION("having a value as the count output should clear the buffer and output the count")
    {
        size_t count = 0;
        manager.drain_additions_input_buffer(nullptr, &count, &max_handle);
        REQUIRE(some_inputted_max_handle == max_handle);
        REQUIRE(1 == count);
        REQUIRE(0 == manager.additions_input.count);
    }
}

TEST_CASE("draining the additions input buffer should drain the items into a specified output", "[thread communication][additions input buffer][drain][read][write]")
{
    SomeEntryType some_data[4] = {
        {.item = 9, .handle = 0},
        {.item = 3, .handle = 1},
        {.item = 6, .handle = 2},
        {.item = 1, .handle = 3},
    };

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_input.count);

    manager.add_to_additions_input_buffer(some_data, 4, 0);

    REQUIRE(4 == manager.additions_input.count);

    SomeEntryType* entries_output;
    size_t count;
    SomeHandle max_handle;

    manager.drain_additions_input_buffer(&entries_output, &count, &max_handle);

    REQUIRE(nullptr != entries_output);
    REQUIRE(4 == count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i].item == entries_output[i].item);
        REQUIRE(some_data[i].handle == entries_output[i].handle);
    }
    REQUIRE(0 == manager.additions_output.count);

    delete[] entries_output;
}

TEST_CASE("draining the additions output buffer with nullptr as a specified output should clear the additions output buffer", "[thread communication][additions output buffer][drain][write]")
{
    SomeHandle some_data[4] = {1, 2, 67, 4};
    SomeHandle some_inputted_max_handle = 3;

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_output.count);

    manager.add_to_additions_output_buffer(some_data, 4, some_inputted_max_handle);
    REQUIRE(4 == manager.additions_output.count);

    SECTION("having nullptr as the count output should clear the buffer")
    {
        SomeHandle max_handle = manager.drain_additions_output_buffer(nullptr, nullptr);
        REQUIRE(some_inputted_max_handle == max_handle);
        CHECK(0 == manager.additions_output.count);
    }

    SECTION("having a value as the count output should clear the buffer and output the count")
    {
        size_t count = 0;
        SomeHandle max_handle = manager.drain_additions_output_buffer(nullptr, &count);
        REQUIRE(some_inputted_max_handle == max_handle);
        REQUIRE(4 == count);
        CHECK(0 == manager.additions_output.count);
    }

}

TEST_CASE("draining the additions output buffer should drain the items into a specified output", "[thread communication][additions output buffer][drain][read][write]")
{
    SomeHandle some_data[4] = {67, 21, 32, 64};
    SomeHandle some_max_handle = 3;

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.additions_input.count);

    manager.add_to_additions_output_buffer(some_data, 4, some_max_handle);

    REQUIRE(4 == manager.additions_output.count);
    REQUIRE(some_max_handle == manager.additions_output.max_handle);

    SomeHandle* entries_output;
    size_t count;
    SomeHandle max_handle;

    max_handle = manager.drain_additions_output_buffer(&entries_output, &count);

    REQUIRE(nullptr != entries_output);
    REQUIRE(4 == count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i] == entries_output[i]);
    }
    REQUIRE(0 == manager.additions_output.count);
    REQUIRE(some_max_handle == max_handle);

    delete[] entries_output;
}

TEST_CASE("draining the removals input buffer with nullptr as a specified input should clear the removals input buffer", "[thread communication][removals input buffer][drain][write]")
{
    SomeHandle some_data[4] = {1, 2, 67, 4};

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.removals_input.count);

    manager.add_to_removals_input_buffer(some_data, 4);
    REQUIRE(4 == manager.removals_input.count);

    SECTION("having nullptr as the count output should clear the buffer")
    {
        manager.drain_removals_input_buffer(nullptr, nullptr);
        CHECK(0 == manager.removals_input.count);
    }

    SECTION("having a value as the count output should clear the buffer and output the count")
    {
        size_t count = 0;
        manager.drain_removals_input_buffer(nullptr, &count);
        REQUIRE(4 == count);
        CHECK(0 == manager.additions_output.count);
    }
}

TEST_CASE("draining the removals input buffer should drain the items into a specified input", "[thread communication][removals input buffer][drain][read][write]")
{
    SomeHandle some_data[4] = {67, 21, 32, 64};

    ThreadCommunication::BufferManager<SomeType, SomeHandle, SomeEntryType> manager;
    REQUIRE(0 == manager.removals_input.count);

    manager.add_to_removals_input_buffer(some_data, 4);

    REQUIRE(4 == manager.removals_input.count);

    SomeHandle* entries_output;
    size_t count;

    manager.drain_removals_input_buffer(&entries_output, &count);

    REQUIRE(nullptr != entries_output);
    REQUIRE(4 == count);
    for (size_t i = 0; i < 4; ++i)
    {
        REQUIRE(some_data[i] == entries_output[i]);
    }
    REQUIRE(0 == manager.removals_input.count);

    delete[] entries_output;
}