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

TEST_CASE("persistent items should be able to be added to the residency manager", "[residency manager][residency][persistent residency][add][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();

    SECTION("persistent buffers should be able to be added to the residency manager")
    {
        MTL::Buffer* some_buffer = device->newBuffer(128, MTL::ResourceStorageModeShared);

        ResidencyManager* residency_manager = new ResidencyManager(device, command_queue);
        REQUIRE(0 == residency_manager->persistent_allocations_count());

        residency_manager->add_persistent(some_buffer);
        REQUIRE(1 == residency_manager->persistent_allocations_count());

        delete residency_manager;
        some_buffer->release();
    }

    SECTION("persistent textures should be able to be added to the residency manager")
    {
        ResidencyManager* residency_manager = new ResidencyManager(device, command_queue);
        MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
        texture_descriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
        texture_descriptor->setTextureType(MTL::TextureType2D);
        texture_descriptor->setWidth(128);
        texture_descriptor->setHeight(128);
        texture_descriptor->setStorageMode(MTL::StorageModeShared);
        texture_descriptor->setUsage(MTL::TextureUsageShaderRead);

        MTL::Texture* some_texture = device->newTexture(texture_descriptor);
        texture_descriptor->release();
        REQUIRE(0 == residency_manager->persistent_allocations_count());

        residency_manager->add_persistent(some_texture);
        REQUIRE(1 == residency_manager->persistent_allocations_count());

        delete residency_manager;
        some_texture->release();
    }

    SECTION("persistent heaps should be able to be added to the residency manager")
    {
        ResidencyManager* residency_manager = new ResidencyManager(device, command_queue);
        MTL::HeapDescriptor* heap_descriptor = MTL::HeapDescriptor::alloc()->init();
        heap_descriptor->setSize(1024);
        heap_descriptor->setStorageMode(MTL::StorageModeShared);
        heap_descriptor->setType(MTL::HeapTypeAutomatic);

        MTL::Heap* some_heap = device->newHeap(heap_descriptor);
        heap_descriptor->release();
        REQUIRE(0 == residency_manager->persistent_allocations_count());

        residency_manager->add_persistent(some_heap);
        REQUIRE(1 == residency_manager->persistent_allocations_count());

        delete residency_manager;
        some_heap->release();
    }

    command_queue->release();
    autorelease_pool->release();
    device->release();
}

TEST_CASE("dynamic items should be able to be added and removed from the residency manager", "[residency manager][residency][dynamic residency][add][remove][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();

    SECTION("buffers should be able to be added to and removed from the residency manager")
    {
        ResidencyManager residency_manager(device, command_queue);
        MTL::Buffer* some_buffer = device->newBuffer(128, MTL::ResourceStorageModeShared);
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        residency_manager.add_dynamic(some_buffer);
        REQUIRE(1 == residency_manager.dynamic_allocations_count());

        residency_manager.remove_dynamic(some_buffer);
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        some_buffer->release();
    }

    SECTION("textures should be able to be added to and removed from the residency manager")
    {
        ResidencyManager residency_manager(device, command_queue);
        MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
        texture_descriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
        texture_descriptor->setTextureType(MTL::TextureType2D);
        texture_descriptor->setWidth(128);
        texture_descriptor->setHeight(128);
        texture_descriptor->setStorageMode(MTL::StorageModeShared);
        texture_descriptor->setUsage(MTL::TextureUsageShaderRead);

        MTL::Texture* some_texture = device->newTexture(texture_descriptor);
        texture_descriptor->release();
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        residency_manager.add_dynamic(some_texture);
        REQUIRE(1 == residency_manager.dynamic_allocations_count());

        residency_manager.remove_dynamic(some_texture);
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        some_texture->release();
    }

    SECTION("heaps should be able to be added to and removed from the residency manager")
    {
        ResidencyManager residency_manager(device, command_queue);
        MTL::HeapDescriptor* heap_descriptor = MTL::HeapDescriptor::alloc()->init();
        heap_descriptor->setSize(1024);
        heap_descriptor->setStorageMode(MTL::StorageModeShared);
        heap_descriptor->setType(MTL::HeapTypeAutomatic);

        MTL::Heap* some_heap = device->newHeap(heap_descriptor);
        heap_descriptor->release();
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        residency_manager.add_dynamic(some_heap);
        REQUIRE(1 == residency_manager.dynamic_allocations_count());

        residency_manager.remove_dynamic(some_heap);
        REQUIRE(0 == residency_manager.dynamic_allocations_count());

        some_heap->release();
    }

    command_queue->release();
    autorelease_pool->release();
    device->release();
}



TEST_CASE("commiting resources through the residency manager should increment the latest commit value", "[residency manager][residency][add][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    MTL4::CommandQueue* command_queue = device->newMTL4CommandQueue();

    ResidencyManager residency_manager(device, command_queue);
    REQUIRE(0 == residency_manager.latest_commit_value);

    residency_manager.commit();
    REQUIRE(0 < residency_manager.latest_commit_value);
    uint64_t previous_latest_commit_value = residency_manager.latest_commit_value;

    residency_manager.commit();
    REQUIRE(previous_latest_commit_value < residency_manager.latest_commit_value);

    command_queue->release();
    autorelease_pool->release();
    device->release();
}