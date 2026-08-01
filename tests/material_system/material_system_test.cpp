/**
 * @file material_system_test.cpp
 * @brief Unit tests for the material system
 */

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "asset_systems/material_system.hpp"
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
            .emission = nullptr,
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

TEST_CASE("Adding n materials should increase the number of materials by n", "[material][asset system][add]")
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
            .emission = nullptr,
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