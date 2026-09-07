#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/residency_manager.hpp"

TEST_CASE("residency manager should sucessfully create residency sets on init", "[residency manager][residency][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();

    autorelease_pool->release();
    device->release();
}