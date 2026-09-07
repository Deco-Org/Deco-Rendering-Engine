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

    REQUIRE(0 == command_allocator_pool->get_frame_count());

    command_allocator_pool->begin_frame();
    REQUIRE(1 == command_allocator_pool->get_frame_count());

    MTL4::CommandBuffer* command_buffer = command_allocator_pool->get_command_buffer();
    command_queue->commit(&command_buffer, 1);
    command_allocator_pool->signal_frame_complete(command_queue);

    command_allocator_pool->begin_frame();
    REQUIRE(2 == command_allocator_pool->get_frame_count());

    command_queue->commit(&command_buffer, 1);
    command_allocator_pool->signal_frame_complete(command_queue);

    delete command_allocator_pool;
    command_queue->release();
    autorelease_pool->release();
    device->release();
}