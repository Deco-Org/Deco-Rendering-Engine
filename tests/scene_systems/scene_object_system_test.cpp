#include <catch2/catch_test_macros.hpp>
#include "test_utils.hpp"
#include "scene_systems/scene_object_system.hpp"
#include "scene_object_system_test_fixture.hpp"

static const SceneObjectSystemConfig system_config = {
    .submesh_system = nullptr,
    .transformation_system = nullptr,
    // .animation_system = nullptr,
    .material_system = nullptr,
};

TEST_CASE("adding one scene object without draining the input buffer should not change the number of scene objects", "[scene object system][scene system][add]")
{
    SceneObjectSystem system(system_config);
    REQUIRE(0 == system.count());

    system.add((SceneObject){
        .submesh_handle = 0,
        .transformation_handle = 0,
        .material_handle = 0,
    });

    REQUIRE(0 == system.count());
}

TEST_CASE("adding one scene object and draining the input buffer should increment the number of scene objects by one", "[scene object system][scene system][add]")
{
    SceneObjectSystem system(system_config);
    REQUIRE(0 == system.count());

    system.add((SceneObject){
        .submesh_handle = 0,
        .transformation_handle = 0,
        .material_handle = 0,
    });

    system.drain_additions_input_buffer();
    REQUIRE(1 == system.count());

    system.get_items_and_drain_output_buffer();
    system.add((SceneObject){
        .submesh_handle = 1,
        .transformation_handle = 1,
        .material_handle = 1,
    });

    system.drain_additions_input_buffer();
    REQUIRE(2 == system.count());

    system.get_items_and_drain_output_buffer();
}

TEST_CASE("adding n scene objects should increment the number of scene objects by n", "[scene object system][scene system][add]")
{
    SceneObjectSystem system(system_config);

    SceneObject scene_objects[] = {
        { .submesh_handle = 0, .transformation_handle = 0, .material_handle = 0 },
        { .submesh_handle = 1, .transformation_handle = 1, .material_handle = 1 },
    };

    REQUIRE(0 == system.count());

    system.add({
        .data = scene_objects,
        .count = 2
    });

    system.drain_additions_input_buffer();
    REQUIRE(2 == system.count());
    
    system.get_items_and_drain_output_buffer();
}

TEST_CASE("added scene objects should have correct static sort key parts", "[scene object system][scene system][sorting][add][fbx][metal]")
{
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    TextureLoader* texture_loader = new TextureLoader(device);
    MaterialSystem* material_system = new MaterialSystem(texture_loader);
    SceneObjectSystemConfig system_config_with_working_material_system = {
        .submesh_system = nullptr,
        .material_system = material_system,
        .transformation_system = nullptr,
    };
    SceneObjectSystem scene_object_system(system_config_with_working_material_system);
    ufbx_scene* scene = a_scene_with_a_test_lamp_model();

    SECTION("sort key parts should generate correctly for opaque scene objects")
    {
        std::vector<MaterialHandle> material_handles = material_system->add(&scene->materials, MaterialProperties::Opaque);
        REQUIRE(0 < material_handles.size());
        material_system->drainAdditionsInputBuffer();
        material_system->getItemsAndDrainAdditionsOutputBuffer();
        
        SceneObject scene_object = {
            .submesh_handle = INVALID_SUBMESH_HANDLE,
            .material_handle = material_handles[0],
            .transformation_handle = TRANSFORMATION_HANDLE_INVALID,
        };

        std::vector<SceneObjectHandle> scene_object_handles = scene_object_system.add({
            .data = &scene_object,
            .count = 1,
        });
        REQUIRE(0 == scene_object_system.count());
        scene_object_system.drain_additions_input_buffer();
        CAPTURE(scene_object_handles);
        const uint64_t sort_key = scene_object_system.static_sort_key_parts[scene_object_handles[0]];
        CAPTURE(sort_key);
        SECTION("sort key ordering and parts should reflect material opacity")
        {
            MaterialProperties actual_properties = static_cast<MaterialProperties>(sort_key << SortKeyBitMappings::Shifts::Transparency);
            REQUIRE(MaterialProperties::Opaque == actual_properties);
        }
    
        SECTION("sort key should reflect pipeline flags")
        {
            RenderPipelineFlags pipeline_flags = static_cast<RenderPipelineFlags>(sort_key << static_cast<uint64_t>(SortKeyBitMappings::Shifts::OpaqueShifts::PipelineFlags));
            REQUIRE(RenderPipelineFlags::None == pipeline_flags);
        }
    
        SECTION("sort key should reflect material handle")
        {
            uint64_t expected = get_final_n_bits(scene_object.material_handle, SortKeyBitMappings::Widths::MaterialHandle);
            uint64_t actual = get_final_n_bits(
                sort_key >> SortKeyBitMappings::Shifts::OpaqueShifts::MaterialHandle,
                SortKeyBitMappings::Widths::MaterialHandle
            );
            REQUIRE(expected == actual);
        }
    
        SECTION("sort key should reflect submesh handle")
        {
            uint64_t expected = get_final_n_bits(scene_object.submesh_handle, SortKeyBitMappings::Widths::SubmeshHandle);
            uint64_t actual = get_final_n_bits(
                sort_key >> SortKeyBitMappings::Shifts::OpaqueShifts::SubmeshHandle,
                SortKeyBitMappings::Widths::SubmeshHandle
            );
            REQUIRE(expected == actual);
        }

        scene_object_system.remove({
            .data = scene_object_handles.data(),
            .count = scene_object_handles.size()
        });
        scene_object_system.drain_removals_input_buffer();
        material_system->remove({
            .data = material_handles.data(),
            .count = material_handles.size()
        });
        material_system->drainRemovalsInputBuffer();
        material_system->drainRemovalsOutputBufferAndUnloadResources();
    }

    SECTION("sort key parts should generate correctly for scene objects supporting transparency")
    {

    }

    delete material_system;
    delete texture_loader;

    a_scene_is_freed(scene);

    device->release();
    autorelease_pool->release();
}

TEST_CASE("removing a scene object and draining the input buffer should decrement the number of scene objects by one" "[scene object system][scene system][remove]")
{
    SceneObjectSystem system(system_config);
    REQUIRE(0 == system.count());

    SceneObject sceneObjectsToAdd[] = {
        {
            .submesh_handle = 0,
            .transformation_handle = 0,
            .material_handle = 0,
        },
        {
            .submesh_handle = 1,
            .transformation_handle = 1,
            .material_handle = 1,
        },
    };

    std::vector<SceneObjectHandle> added_scene_objects = system.add({
        .data = sceneObjectsToAdd,
        .count = 2
    });

    system.drain_additions_input_buffer();
    system.get_items_and_drain_output_buffer();

    REQUIRE(2 == system.count());

    system.remove(added_scene_objects[0]);

    CAPTURE(added_scene_objects);
    REQUIRE(2 == system.count());

    system.drain_removals_input_buffer();
    
    REQUIRE(1 == system.count());

    system.remove(added_scene_objects[1]);
    system.drain_removals_input_buffer();

    REQUIRE(0 == system.count());
}