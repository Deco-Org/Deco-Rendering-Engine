#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/residency_manager.hpp"

TEST_CASE("residency manager should successfully init and deconstruct", "[residency manager][residency][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();

    {
        ResidencyManager residency_manager(device, command_queue);
        REQUIRE(0 == residency_manager.latest_commit_value);
    }

    command_queue->release();
    autorelease_pool->release();
    device->release();
}
