#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "metal_backend/render_pipeline_library.hpp"

TEST_CASE("all pipelines build without errors", "[pipeline][render][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CompilerDescriptor* compiler_descriptor = MTL4::CompilerDescriptor::alloc()->init();
    NS::Error* compiler_creation_error = nullptr;
    MTL4::Compiler* compiler = device->newCompiler(compiler_descriptor, &compiler_creation_error);
    REQUIRE(nullptr == compiler_creation_error);
    compiler_descriptor->release();
    REQUIRE(compiler);

    RenderPipelineLibrary library(device, compiler);
    library.build_formats(MTL::PixelFormatBGRA8Unorm);

    for (int i = 0; i < (int)RenderPipelineFlags::FlagCount; ++i)
    {
        REQUIRE(library.get((RenderPipelineHandle)i) != nullptr);
    }

    compiler->release();
    autorelease_pool->release();
}