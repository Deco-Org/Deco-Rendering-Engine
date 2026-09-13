#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/shader_loader.hpp"

TEST_CASE("shaders are loaded without errors", "[shader][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();

    MTL::Library* library = nullptr;
    library = ShaderLoader::load_shader_library(device);
    REQUIRE(nullptr != library);
    REQUIRE(0 < library->functionNames()->count());

    autorelease_pool->release();
    device->release();
}