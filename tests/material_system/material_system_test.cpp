/**
 * @file material_system_test.cpp
 * @brief Unit tests for the material system
 */

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "asset_systems/material_system.hpp"
#include "material_system_test_fixture.hpp"
#include "test_utils.hpp"

TEST_CASE("adding a PBR material should increase the number of materials by one", "[material][asset system][add]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    MaterialEntry addedPBRMaterial = {
        .type = MaterialType::PBR,
        .pbrMaterial = (PBRMaterial){
            .albedoTexture = nullptr,
            .normalTexture = nullptr,
            .metallicRoughnessAoTexture = nullptr,
            .emissionTexture = nullptr,
            .baseColorFactor = simd_float4{1.0f, 1.0f, 1.0f, 1.0f},
        },
    };

    std::vector<MaterialHandle> addedMaterials = system.add((MaterialEntryList) {
        .data = &addedPBRMaterial,
        .count = 1
    });

    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(1 == system.materials.size());
}

TEST_CASE("adding a PBR material from ufbx without textures should increase the number of materials by one", "[material][asset system][add][fbx]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    ufbx_material* material = new ufbx_material();
    material->element = {
        .name = {
            .data = "Material",
            .length = 9
        }
    };
    material->pbr = (ufbx_material_pbr_maps) {
        (ufbx_material_map) {
            // Base Color
            .value_vec3 = {0.8f, 0.8f, 0.8f},
            .texture = nullptr,
            .texture_enabled = false,
            .feature_disabled = false,
            .value_components = 3
        },
    };

    ufbx_material_list materialsToInsert = {
        .data = &material,
        .count = 1
    };

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(1 == system.materials.size());

    delete material;
}

TEST_CASE("adding a toon material should increase the number of materials by one", "[material][asset system][add]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    MaterialEntry addedToonMaterial = {
        .type = MaterialType::Toon,
        .toonMaterial = (ToonMaterial) {
            .albedoTexture = nullptr,
            .shadowThresholdTexture = nullptr,
            .baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f },
            .shadowSoftness = 0.0f
        }
    };

    std::vector<MaterialHandle> addedMaterials = system.add((MaterialEntryList) {
        .data = &addedToonMaterial,
        .count = 1
    });

    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(1 == system.materials.size());
    REQUIRE(addedMaterials.size() - 1 == system.largestHandle);
}

TEST_CASE("adding n materials should increase the number of materials by n", "[material][asset system][add]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    MaterialEntry addedToonMaterial = {
        .type = MaterialType::Toon,
        .toonMaterial = (ToonMaterial) {
            .albedoTexture = nullptr,
            .shadowThresholdTexture = nullptr,
            .baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f },
            .shadowSoftness = 0.0f
        }
    };

    MaterialEntry addedPBRMaterial = {
        .type = MaterialType::PBR,
        .pbrMaterial = (PBRMaterial){
            .albedoTexture = nullptr,
            .normalTexture = nullptr,
            .metallicRoughnessAoTexture = nullptr,
            .emissionTexture = nullptr,
            .baseColorFactor = simd_float4{1.0f, 1.0f, 1.0f, 1.0f},
        },
    };

    MaterialEntry entries[2] = {addedToonMaterial, addedPBRMaterial};
    
    std::vector<MaterialHandle> handles = system.add((MaterialEntryList) {
        .data = entries,
        .count = 2
    });

    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(2 == system.materials.size());
    REQUIRE(handles.size() == system.materials.size());
    REQUIRE(handles.size() - 1 == system.largestHandle);
}

TEST_CASE("adding n materials from ufbx without textures should increase the number of materials by n", "[material][asset system][add][fbx]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    ufbx_material_list materialsToInsert = nUntexturedUfbxMaterials(4);

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(4 == system.materials.size());

    someUfbxMaterialsAreFreed(materialsToInsert);
}

TEST_CASE("adding n materials and draining the additions input buffer should put n handles into the output buffer", "[material][asset system][add]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    ufbx_material_list materialsToInsert = nUntexturedUfbxMaterials(4);

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();

    REQUIRE(4 == system.materials.size());

    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    REQUIRE(4 == consumedHandles.size());

    someUfbxMaterialsAreFreed(materialsToInsert);
}

TEST_CASE("materials added using ufbx materials without textures should have same material information as ufbx materials", "[material][asset system][add][fbx]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    ufbx_scene* scene = aSceneWithATestLampModel();
    ufbx_material_list materialsToInsert = scene->nodes[2]->materials;

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    REQUIRE(materialsToInsert.count == system.materials.size());
    for (size_t i = 0; i < materialsToInsert.count; ++i)
    {
        untexturedMaterialInSystemShouldMatchUfbxMaterial(
            system,
            addedMaterials[i],
            materialsToInsert[i],
            MaterialType::PBR
        );
    }

    aSceneIsFreed(scene);
}

TEST_CASE("materials added using ufbx without textures should resolve issues of value component counts not matching engine component counts", "[material][asset system][add][fbx]") {}

TEST_CASE("a ufbx material with a texture being added through should result in a material with the specified texture", "[material][asset system][add][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    REQUIRE(nullptr != system.materials[handles[albedoMaterialIndices[0]]].pbrMaterial.albedoTexture);
    REQUIRE(nullptr != system.materials[handles[normalMaterialIndices[0]]].pbrMaterial.normalTexture);
    REQUIRE(nullptr != system.materials[handles[emissionMaterialIndices[0]]].pbrMaterial.emissionTexture);
    
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    system.remove((MaterialHandleList) {
        .data = consumedHandles.data(),
        .count = consumedHandles.size()
    });
    
    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("adding a ufbx material without a diffuse color value should result in a material with a default albedo", "[material][asset system][add][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());

    cubeNode->materials.data[albedoMaterialIndices[0]]->fbx.diffuse_color.has_value = false;
    cubeNode->materials.data[albedoMaterialIndices[0]]->fbx.diffuse_color.texture_enabled = false;
    cubeNode->materials.data[albedoMaterialIndices[0]]->fbx.diffuse_color.value_components = 0;
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    REQUIRE(nullptr == system.materials[handles[albedoMaterialIndices[0]]].pbrMaterial.albedoTexture);
    REQUIRE(simdFloat4Equal(DEFAULT_COLOR_4_CHANNELS, system.materials[handles[albedoMaterialIndices[0]]].pbrMaterial.baseColorFactor));

    REQUIRE(nullptr != system.materials[handles[normalMaterialIndices[0]]].pbrMaterial.normalTexture);
    REQUIRE(nullptr != system.materials[handles[emissionMaterialIndices[0]]].pbrMaterial.emissionTexture);
    
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    system.remove((MaterialHandleList) {
        .data = consumedHandles.data(),
        .count = consumedHandles.size()
    });
    
    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("removing a material from the system should free the handle", "[material][asset system][remove]")
{
    TextureLoader textureLoader;
    MaterialSystem system(&textureLoader);

    ufbx_material_list materialsToInsert = nUntexturedUfbxMaterials(4);

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    REQUIRE(4 == system.materials.size());

    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle removedHandle = consumedHandles[1];

    // When an item is removed
    system.remove((MaterialHandleList) {
        .data = &removedHandle,
        .count = 1
    });
    REQUIRE(4 == system.materials.size());
    
    system.drainRemovalsInputBuffer();
    REQUIRE(system.freeHandles.size() == 0);
    
    system.drainRemovalsOutputBufferAndUnloadResources();
    REQUIRE(removedHandle == system.freeHandles[system.freeHandles.size() - 1]);
    REQUIRE(3 == system.materials.size() - system.freeHandles.size());

    someUfbxMaterialsAreFreed(materialsToInsert);
}

TEST_CASE("removing a material from the system should decrement the use count of the texture when the removals output buffer is drained", "[material][asset system][remove][texture][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);

    ufbx_texture texture = {
        .type = UFBX_TEXTURE_FILE,
        .filename = (ufbx_string) {
            .data = "assets/test_cube_texture.png",
            .length = 22
        }
    };

    ufbx_material_list materialsToInsert = nUntexturedUfbxMaterials(4);
    materialsToInsert[1]->fbx.diffuse_color.has_value = true;
    materialsToInsert[1]->fbx.diffuse_color.texture_enabled = true;
    materialsToInsert[1]->fbx.diffuse_color.texture = &texture;
    materialsToInsert[1]->fbx.diffuse_color.texture->has_file = true;

    std::vector<MaterialHandle> addedMaterials = system.add(&materialsToInsert);
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    REQUIRE(4 == system.materials.size());
    REQUIRE(nullptr != system.materials[addedMaterials[1]].pbrMaterial.albedoTexture);
    REQUIRE(1 == textureLoader.getUseCount(system.materials[addedMaterials[1]].pbrMaterial.albedoTexture));
    
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle removedHandle = consumedHandles[1];
    
    // When an item is removed
    system.remove((MaterialHandleList) {
        .data = &removedHandle,
        .count = 1
    });
    REQUIRE(4 == system.materials.size());
    
    system.drainRemovalsInputBuffer();
    REQUIRE(system.freeHandles.size() == 0);
    
    system.drainRemovalsOutputBufferAndUnloadResources();
    REQUIRE(removedHandle == system.freeHandles[system.freeHandles.size() - 1]);
    REQUIRE(3 == system.materials.size() - system.freeHandles.size());
    REQUIRE(0 == textureLoader.getUseCount(system.materials[addedMaterials[1]].pbrMaterial.albedoTexture));
    
    someUfbxMaterialsAreFreed(materialsToInsert);

    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("material handles should be recycled when a handle is removed", "[material][asset system][add][remove][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle handleToBeRecycled = consumedHandles[1];
    REQUIRE(false == system.materials[handleToBeRecycled].isTombstone);

    system.remove((MaterialHandleList){.data = &handleToBeRecycled, .count = 1});
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();

    REQUIRE(true == system.materials[handleToBeRecycled].isTombstone);

    std::vector<MaterialHandle> newlyAddedHandles = system.add(&cubeNode->materials, MaterialType::PBR);
    REQUIRE(handleToBeRecycled == newlyAddedHandles[0]);

    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("updating a material should update the material within the system", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle materialToUpdate = consumedHandles[1];

    simd_float4 updatedBaseColor = {0.431f, 0.835f, 0.961f, 1.0f};

    MaterialEntry newMaterial = {
        .type = MaterialType::PBR,
        .pbrMaterial = {
            .albedoTexture = nullptr,
            .normalTexture = nullptr,
            .metallicRoughnessAoTexture = nullptr,
            .emissionTexture = nullptr,

            .baseColorFactor = updatedBaseColor,
            .metallicFactor = 1.0f,
            .roughnessFactor = 1.0f,
            .ambientOcclusionFactor = 1.0f,
            .emissionColorAndFactor = { 0.0f, 0.0f, 0.0f, 0.0f }
        }
    };

    // When the call is made to update the material
    system.updateMaterial(materialToUpdate, newMaterial);
    // The updates input buffer should have one item
    REQUIRE(1 == system.updatesInputBuffer.count);

    // When the updates input buffer is drained
    system.drainUpdatesInputBuffers();
    // The updates input buffer should have zero items
    REQUIRE(0 == system.updatesInputBuffer.count);

    REQUIRE(false == system.materials[materialToUpdate].isTombstone);
    REQUIRE(simdFloat4Equal(updatedBaseColor, system.materials[materialToUpdate].pbrMaterial.baseColorFactor));
    REQUIRE(nullptr == system.materials[materialToUpdate].pbrMaterial.albedoTexture);
    REQUIRE(nullptr == system.materials[materialToUpdate].pbrMaterial.normalTexture);
    REQUIRE(nullptr == system.materials[materialToUpdate].pbrMaterial.metallicRoughnessAoTexture);
    REQUIRE(nullptr == system.materials[materialToUpdate].pbrMaterial.emissionTexture);

    REQUIRE(0 != system.texturesToUnloadBuffer.count);
    system.unloadUnusedReplacedTextures();
    REQUIRE(0 == system.texturesToUnloadBuffer.count);

    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("updating a material by adding a texture should be reflected in the system and should increment the use count of the new texture", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle materialToUpdate = consumedHandles[1];

    MTL::Texture* textureToAdd = textureLoader.loadTexture("assets/test_cube_texture.png");
    REQUIRE(system.materials[materialToUpdate].pbrMaterial.metallicRoughnessAoTexture != textureToAdd);

    // When the call is made to update the material texture
    system.updateMaterialTexture(materialToUpdate, (MaterialTextureOffset::TextureOffset)MaterialTextureOffset::PBRTextureOffset::ORM, textureToAdd);
    // The texture updates input buffer should have one item
    REQUIRE(1 == system.textureUpdatesInputBuffer.count);
    REQUIRE(textureToAdd != system.materials[materialToUpdate].pbrMaterial.metallicRoughnessAoTexture);

    // When the updates input buffer is drained
    system.drainUpdatesInputBuffers();
    // The texture updates input buffer should have zero items
    REQUIRE(0 == system.textureUpdatesInputBuffer.count);

    REQUIRE(textureToAdd == system.materials[materialToUpdate].pbrMaterial.metallicRoughnessAoTexture);

    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("no textures should be removed when the update input buffers are empty when drained", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());

    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));

    system.drainAdditionsInputBuffer();
    system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle someMaterialHandle = handles[1];
    MTL::Texture* oldTexture = system.materials[someMaterialHandle].pbrMaterial.albedoTexture;

    REQUIRE(0 == system.updatesInputBuffer.count);

    system.drainUpdatesInputBuffers();
    REQUIRE(0 == system.updatesInputBuffer.count);

    REQUIRE(false == system.materials[someMaterialHandle].isTombstone);
    REQUIRE(simdFloat4Equal(DEFAULT_COLOR_4_CHANNELS, system.materials[someMaterialHandle].pbrMaterial.baseColorFactor));
    REQUIRE(oldTexture == system.materials[someMaterialHandle].pbrMaterial.albedoTexture);

    
    // Cleaning up
    system.unloadUnusedReplacedTextures();
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("texture updates should take precedence over material updates when a material is both updated entirely and has a texture updated before the update buffers are drained", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle materialToUpdate = consumedHandles[1];

    MTL::Texture* oldTexture = system.materials[materialToUpdate].pbrMaterial.albedoTexture;
    MTL::Texture* textureInNewMaterial = nullptr;

    simd_float4 updatedBaseColor = {0.431f, 0.835f, 0.961f, 1.0f};

    MaterialEntry newMaterial = {
        .type = MaterialType::PBR,
        .pbrMaterial = {
            .albedoTexture = textureInNewMaterial,
            .normalTexture = nullptr,
            .metallicRoughnessAoTexture = nullptr,
            .emissionTexture = nullptr,

            .baseColorFactor = updatedBaseColor,
            .metallicFactor = 1.0f,
            .roughnessFactor = 1.0f,
            .ambientOcclusionFactor = 1.0f,
            .emissionColorAndFactor = { 0.0f, 0.0f, 0.0f, 0.0f }
        }
    };

    MTL::Texture* albedoTextureToAdd = textureLoader.loadTexture("assets/test_cube_four_channel_texture.png");

    system.updateMaterial(materialToUpdate, newMaterial);
    system.updateMaterialTexture(materialToUpdate, (MaterialTextureOffset::TextureOffset)MaterialTextureOffset::PBRTextureOffset::Albedo, albedoTextureToAdd);
    system.drainUpdatesInputBuffers();

    REQUIRE(oldTexture != system.materials[materialToUpdate].pbrMaterial.albedoTexture);
    REQUIRE(textureInNewMaterial != system.materials[materialToUpdate].pbrMaterial.albedoTexture);

    // Cleaning up
    system.unloadUnusedReplacedTextures();
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("updating a material by replacing a texture should be reflected in the system", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    handles.append_range(system.add(&cubeNode->materials, MaterialType::PBR));
    REQUIRE(0 < handles.size());
    REQUIRE(0 == system.materials.size());

    system.drainAdditionsInputBuffer();
    std::vector<MaterialHandle> consumedHandles = system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle materialToUpdate = consumedHandles[1];

    MTL::Texture* textureToAdd = textureLoader.loadTexture("assets/test_cube_four_channel_texture.png");
    REQUIRE(system.materials[materialToUpdate].pbrMaterial.albedoTexture != textureToAdd);

    // When the call is made to update the material texture
    system.updateMaterialTexture(materialToUpdate, (MaterialTextureOffset::TextureOffset)MaterialTextureOffset::PBRTextureOffset::Albedo, textureToAdd);
    // The texture updates input buffer should have one item
    REQUIRE(1 == system.textureUpdatesInputBuffer.count);
    REQUIRE(textureToAdd != system.materials[materialToUpdate].pbrMaterial.albedoTexture);

    // When the updates input buffer is drained
    system.drainUpdatesInputBuffers();
    // The texture updates input buffer should have zero items
    REQUIRE(0 == system.textureUpdatesInputBuffer.count);

    REQUIRE(textureToAdd == system.materials[materialToUpdate].pbrMaterial.albedoTexture);

    // Cleaning up
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}

TEST_CASE("updating a component of the ORM texture of a material should update the ORM texture of the material within the system", "[material][asset system][update][fbx][metal]")
{
    NS::AutoreleasePool* autoReleasePool = NS::AutoreleasePool::alloc()->init();
    MTL::Device* metalDevice = MTL::CreateSystemDefaultDevice();

    TextureLoader textureLoader(metalDevice);
    MaterialSystem system(&textureLoader);
    ufbx_scene* scene = aSceneWithATexturedCube();
    ufbx_node* cubeNode = aNodeWithAGivenNameInAScene(scene, "Cube");
    REQUIRE(nullptr != cubeNode);

    std::vector<size_t> albedoMaterialIndices = indicesInAListWithAnAlbedoTexture(cubeNode->materials);
    std::vector<size_t> normalMaterialIndices = indicesInAListWithANormalTexture(cubeNode->materials);
    std::vector<size_t> emissionMaterialIndices = indicesInAListWithAnEmissionTexture(cubeNode->materials);
    REQUIRE(0 < albedoMaterialIndices.size());
    REQUIRE(0 < normalMaterialIndices.size());
    REQUIRE(0 < emissionMaterialIndices.size());
    
    std::vector<MaterialHandle> handles = system.add(&cubeNode->materials, MaterialType::PBR);
    system.drainAdditionsInputBuffer();
    system.getItemsAndDrainAdditionsOutputBuffer();
    MaterialHandle materialToUpdate = handles[1];
    MTL::Texture* oldTexture = system.materials[materialToUpdate].pbrMaterial.metallicRoughnessAoTexture;

    SECTION("Adding components of an ORM texture to a material should update the material's ORM texture")
    {
        SECTION("Adding an AO texture to a material should update the material's ORM texture") {}
    
        SECTION("Adding a roughness texture to a material should update the material's ORM texture") {}
    
        SECTION("Adding a metallic texture to a material should update the material's ORM texture") {}
    }

    SECTION("Updating components of a material's ORM texture should update the material's ORM texture")
    {
        SECTION("Replacing the AO texture of a material should update the material's ORM texture") {}
    
        SECTION("Replacing the roughness texture of a material should update the material's ORM texture") {}
    
        SECTION("Replacing the metallic texture of a material should update the material's ORM texture") {}
    }

    // Cleaning up
    system.unloadUnusedReplacedTextures();
    system.drainRemovalsInputBuffer();
    system.drainRemovalsOutputBufferAndUnloadResources();
    aSceneIsFreed(scene);
    metalDevice->release();
    autoReleasePool->release();
}