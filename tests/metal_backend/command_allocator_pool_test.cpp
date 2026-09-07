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