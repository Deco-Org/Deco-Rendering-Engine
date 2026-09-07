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

TEST_CASE("the frame count should not increment until the number of frames in flight is less than the maximum", "[command allocator pool][frame][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();
    CommandAllocatorPool* command_allocator_pool = new CommandAllocatorPool(device);
    MTL4::CommandBuffer* command_buffer = command_allocator_pool->get_command_buffer();

    auto waiting_lambda = [](CommandAllocatorPool* instance, uint64_t wait_value, uint64_t wait_time_in_millseconds)
    {
        REQUIRE(Config::MAX_FRAMES_IN_FLIGHT == instance->get_frame_count());
        CommandAllocatorPool::default_waiting_function(instance, wait_value, wait_time_in_millseconds);
    };

    auto callback_lambda = [](CommandAllocatorPool* instance)
    {
        REQUIRE(Config::MAX_FRAMES_IN_FLIGHT == instance->get_frame_count());
        CommandAllocatorPool::default_frame_completion_callback_function(instance);
    };

    REQUIRE(0 == command_allocator_pool->get_frame_count());

    // TODO: Right now, this isn't testing what I want it to test. This test should be adjusted.
    for (uint8_t i = 1; i <= Config::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        command_allocator_pool->begin_frame();
        REQUIRE(i == command_allocator_pool->get_frame_count());

        command_queue->commit(&command_buffer, 1);
        command_allocator_pool->signal_frame_complete(command_queue);
    }

    command_allocator_pool->begin_frame(waiting_lambda, callback_lambda);

    REQUIRE(Config::MAX_FRAMES_IN_FLIGHT + 1 == command_allocator_pool->get_frame_count());

    delete command_allocator_pool;
    command_queue->release();
    autorelease_pool->release();
    device->release();
}