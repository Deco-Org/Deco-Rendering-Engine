#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/command_allocator_pool.hpp"

TEST_CASE("command allocator pool should init successfully", "[command allocator pool][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();

    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(device);

    REQUIRE(nullptr != command_allocator_pool);

    delete command_allocator_pool;

    command_queue->release();
    autorelease_pool->release();
    device->release();
}

TEST_CASE("the command allocator pool should create a command buffer on init", "[command allocator pool][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();
    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(device);

    REQUIRE(nullptr != command_allocator_pool->get_command_buffer());

    delete command_allocator_pool;
    command_queue->release();
    autorelease_pool->release();
    device->release();
}

TEST_CASE("the frame count should be zero on init", "[command allocator pool][frame][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();
    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(device);

    REQUIRE(0 == command_allocator_pool->get_frame_count());

    delete command_allocator_pool;
    command_queue->release();
    autorelease_pool->release();
    device->release();
}

TEST_CASE("the frame count should increment upon the occurrence of the frame event", "[command allocator pool][frame][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();
    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(device);
    MTL4::CommandBuffer* command_buffer = command_allocator_pool->get_command_buffer();

    REQUIRE(0 == command_allocator_pool->get_frame_count());

    command_allocator_pool->begin_frame();
    REQUIRE(1 == command_allocator_pool->get_frame_count());
    
    command_queue->commit(&command_buffer, 1);
    command_allocator_pool->signal_frame_complete(command_queue);
    REQUIRE(1 == command_allocator_pool->get_frame_count());

    command_allocator_pool->begin_frame();
    REQUIRE(2 == command_allocator_pool->get_frame_count());

    command_queue->commit(&command_buffer, 1);
    command_allocator_pool->signal_frame_complete(command_queue);

    delete command_allocator_pool;
    command_queue->release();
    autorelease_pool->release();
    device->release();
}

TEST_CASE("the command command allocator pool should wait when the frame count is greater than or equal to the maximum number of frames in flight", "[command allocator pool][frame]")
{
    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(nullptr);

    bool wait_was_called = false;
    uint64_t value_waited_for = 0;

    auto mock_waiting_lambda = [&](CommandAllocatorPool* instance, uint64_t wait_value, uint64_t wait_time_in_milliseconds)
    {
        value_waited_for = wait_value;
        wait_was_called = true;
    };

    auto mock_callback_lambda = [](CommandAllocatorPool* instance) {};

    for (uint8_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        command_allocator_pool->begin_frame(mock_waiting_lambda, mock_callback_lambda);
        REQUIRE(!wait_was_called);
    }

    REQUIRE(Config::MAX_FRAMES_IN_FLIGHT == command_allocator_pool->get_frame_count());
    command_allocator_pool->begin_frame(mock_waiting_lambda, mock_callback_lambda);
    REQUIRE(wait_was_called);
    REQUIRE(0 == value_waited_for);

    wait_was_called = false;
    command_allocator_pool->begin_frame(mock_waiting_lambda, mock_callback_lambda);
    REQUIRE(wait_was_called);
    REQUIRE(1 == value_waited_for);

    delete command_allocator_pool;
}