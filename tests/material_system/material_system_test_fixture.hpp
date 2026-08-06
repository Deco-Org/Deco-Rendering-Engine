/**
 * @file material_system_test_fixture.hpp
 * @brief
 */

#pragma once
#include "asset_systems/material_system.hpp"
#include "test_utils.hpp"

ufbx_material_list nUntexturedUfbxMaterials(size_t n)
{
    ufbx_material_list materialsList = {
        .data = new ufbx_material*[n],
        .count = n
    };

    for (size_t i = 0; i < n; ++i)
    {
        ufbx_material* material = new ufbx_material();
        float colorVal = ((float)i / n);
        material->element.name = {
            .data = "Material",
            .length = 9
        };
        material->pbr = (ufbx_material_pbr_maps) {
            (ufbx_material_map) {
                // Base Color
                .value_vec3 = {colorVal, 1.0f - colorVal, colorVal},
                .texture = nullptr,
                .has_value = true,
                .texture_enabled = false,
                .feature_disabled = false,
                .value_components = 3
            },
        };
        ufbxMaterialFeature(material, pbr) = (ufbx_material_feature_info) {
            .enabled = true,
            .is_explicit = true
        };
        material->features = {
            .features = {
                (ufbx_material_feature_info) { // PBR
                    .enabled = true
                },
                (ufbx_material_feature_info) { // metalness
                    .enabled = false
                },
                (ufbx_material_feature_info) { // diffuse
                    .enabled = false
                },
                (ufbx_material_feature_info) { // specular
                    .enabled = false
                },
                (ufbx_material_feature_info) { // emission
                    .enabled = false
                },
                (ufbx_material_feature_info) { // transmission
                    .enabled = false
                },
                (ufbx_material_feature_info) { // coat
                    .enabled = false
                },
                (ufbx_material_feature_info) { // sheen
                    .enabled = false
                },
                (ufbx_material_feature_info) { // opacity
                    .enabled = false
                },
                (ufbx_material_feature_info) { // ambient_occlusion
                    .enabled = false
                },
                (ufbx_material_feature_info) { // matte
                    .enabled = false
                },
                (ufbx_material_feature_info) { // unlit
                    .enabled = false
                },
                (ufbx_material_feature_info) { // ior
                    .enabled = false
                },
                (ufbx_material_feature_info) { // diffuse_roughness
                    .enabled = false
                },
                (ufbx_material_feature_info) { // transmission_roughness
                    .enabled = false
                },
                (ufbx_material_feature_info) { // thin_walled
                    .enabled = false
                },
                (ufbx_material_feature_info) { // caustics
                    .enabled = false
                },
                (ufbx_material_feature_info) { // exit_to_background
                    .enabled = false
                },
                (ufbx_material_feature_info) { // internal_reflections
                    .enabled = false
                },
                (ufbx_material_feature_info) { // double_sided
                    .enabled = false
                },
                (ufbx_material_feature_info) { // roughness_as_glossiness
                    .enabled = false
                },
                (ufbx_material_feature_info) { // coat_roughness_as_glossiness
                    .enabled = false
                },
                (ufbx_material_feature_info) { // transmission_roughness_as_glossiness
                    .enabled = false
                }
            }
        };
        materialsList[i] = material;
    }

    return materialsList;
}

static ufbx_scene* aSceneWithATestLampModel()
{
    const char filepath[24] = "assets/test_lamp_01.fbx";
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(filepath, &opts, &error);
    REQUIRE(scene);
    return scene;
}

static ufbx_scene* aSceneWithATexturedCube()
{
    const char filepath[24] = "assets/test_cube_02.fbx";
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;
    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(filepath, &opts, &error);
    REQUIRE(scene);
    return scene;
}

static ufbx_node* aNodeWithAGivenNameInAScene(ufbx_scene* scene, std::string name)
{
    for (ufbx_node* node : scene->nodes)
    {
        if (0 == memcmp(node->name.data, name.c_str(), name.length() * sizeof(char)))
            return node;
    }
    return nullptr;
}

static std::vector<size_t> indicesInAListWithAnAlbedoTexture(ufbx_material_list list)
{
    std::vector<size_t> result;
    for (size_t i = 0; i < list.count; ++i)
    {
        ufbx_material* material = list[i];
        if (material->fbx.diffuse_color.has_value)
        {
            result.push_back(i);
        }
    }
    return result;
}

static std::vector<size_t> indicesInAListWithANormalTexture(ufbx_material_list list)
{
    std::vector<size_t> result;
    for (size_t i = 0; i < list.count; ++i)
    {
        ufbx_material* material = list[i];
        if (material->fbx.normal_map.has_value && material->fbx.normal_map.texture_enabled)
        {
            result.push_back(i);
        }
    }
    return result;
}

static std::vector<size_t> indicesInAListWithAnEmissionTexture(ufbx_material_list list)
{
    std::vector<size_t> result;
    for (size_t i = 0; i < list.count; ++i)
    {
        ufbx_material* material = list[i];
        if (material->fbx.emission_color.has_value)
        {
            result.push_back(i);
        }
    }
    return result;
}

static void scalarMaterialValueShouldCorrespondToValuesInMaterialMap(ufbx_material_map map, float actual)
{
    // The result should just be the average
    float total = 0;
    for (int i = 0; i < map.value_components; ++i)
    {
        total += map.value_vec4.v[i];
    }
    const float avg = (map.value_components == 0) ? 1.0f : total / map.value_components;
    CAPTURE(avg, actual);
    REQUIRE(floatsNearEqual(avg, actual));
}

static void float3MaterialValueShouldCorrespondToValuesInMaterialMap(ufbx_material_map map, simd_float3 actual)
{
    switch (map.value_components)
    {
        case 0:
        {
            // If there are zero values, the material should be the default
            REQUIRE(simdFloat3Equal(DEFAULT_COLOR_3_CHANNELS, actual));
        }
        break;

        case 1:
        {
            // If there is one value, all three values of the result material should be equal to that one value
            CAPTURE(map.value_real, actual[0], actual[1], actual[2]);
            REQUIRE(simdFloat3Equal((simd_float3){map.value_real, map.value_real, map.value_real}, actual));
        }
        break;

        case 2:
        {
            // If there are two values, the material should be the default
            CAPTURE(DEFAULT_COLOR_3_CHANNELS[0], DEFAULT_COLOR_3_CHANNELS[1], DEFAULT_COLOR_3_CHANNELS[2], actual[0], actual[1], actual[2]);
            REQUIRE(simdFloat3Equal(DEFAULT_COLOR_3_CHANNELS, actual));
        }
        break;

        case 3:
        {
            // If there are three values, all three slots should match
            CAPTURE(map.value_vec3.x, map.value_vec3.y, map.value_vec3.z, actual[0], actual[1], actual[2]);
            REQUIRE(simdFloat3Equal(
                (simd_float3){
                    map.value_vec3.x,
                    map.value_vec3.y,
                    map.value_vec3.z
                },
                actual
            ));
        }
        break;

        case 4:
        {
            // If there are four values, the alpha should be discarded
            REQUIRE(simdFloat3Equal(
                (simd_float3){
                    map.value_vec4.x,
                    map.value_vec4.y,
                    map.value_vec4.z,
                },
                actual
            ));
        }
        break;

        default:
            break;
    }
}

static void float4MaterialValueShouldCorrespondToValuesInMaterialMap(ufbx_material_map map, simd_float4 actual)
{
    switch (map.value_components)
    {
        case 0:
        {
            // If there are zero values, the result material should be the default
            REQUIRE(simdFloat4Equal(DEFAULT_COLOR_4_CHANNELS, actual));
        }
        break;
        
        case 1:
        {
            // If there is one value, three values of the result material should be equal to that one value, and the fourth should be the default alpha value
            CAPTURE(map.value_real, DEFAULT_COLOR_4_CHANNELS[3], actual[0], actual[1], actual[2], actual[3]);
            REQUIRE(simdFloat4Equal((simd_float4){map.value_real, map.value_real, map.value_real, DEFAULT_COLOR_4_CHANNELS[3]}, actual));
        }
        break;
        
        case 2:
        {
            // If there are two values, the result material should be the default
            REQUIRE(simdFloat4Equal(DEFAULT_COLOR_4_CHANNELS, actual));
        }
        break;
        
        case 3:
        {
            // If there are three values, the result material should have the three values, as well as a fourth value set to the default alpha value
            CAPTURE(map.value_vec3.x, map.value_vec3.y, map.value_vec3.z, DEFAULT_COLOR_4_CHANNELS[3], actual[0], actual[1], actual[2], actual[3]);
            REQUIRE(simdFloat4Equal(
                (simd_float4){
                    map.value_vec3.x,
                    map.value_vec3.y,
                    map.value_vec3.z,
                    DEFAULT_COLOR_4_CHANNELS[3]
                },
                actual
            ));
        }
        break;
        
        case 4:
        {
            // If there are four values, the result material should match
            CAPTURE(map.value_vec4.x, map.value_vec4.y, map.value_vec4.z, map.value_vec4.w, actual[0], actual[1], actual[2], actual[3]);
            REQUIRE(simdFloat4Equal(
                (simd_float4){
                    map.value_vec4.x,
                    map.value_vec4.y,
                    map.value_vec4.z,
                    map.value_vec4.w
                },
                actual
            ));
        }
        break;

        default:
            break;
    }
}

static void untexturedMaterialInSystemShouldMatchUfbxMaterial(MaterialSystem& system, MaterialHandle handle, ufbx_material* material, MaterialType expectedMaterialType)
{
    Material* actual = &system.materials[handle];
    REQUIRE(expectedMaterialType == actual->type);

    switch (actual->type)
    {
    case MaterialType::PBR:
    {
        if (material->fbx.diffuse_color.has_value)
        {
            INFO("Diffuse color");
            float4MaterialValueShouldCorrespondToValuesInMaterialMap(material->fbx.diffuse_color, actual->pbrMaterial.baseColorFactor);
        }

        ufbx_material_map metalnessMaterialMap = material->fbx.specular_exponent;
        if (metalnessMaterialMap.has_value)
        {
            INFO("Metalness Material Map");
            scalarMaterialValueShouldCorrespondToValuesInMaterialMap(metalnessMaterialMap, actual->pbrMaterial.metallicFactor);
        }

        ufbx_material_map roughnessMaterialMap = material->fbx.reflection_factor;
        if (roughnessMaterialMap.has_value)
        {
            INFO("Roughness Material Map");
            scalarMaterialValueShouldCorrespondToValuesInMaterialMap(roughnessMaterialMap, actual->pbrMaterial.roughnessFactor);
        }

        // ufbx_material_map ambientOcclusionMaterialMap = material->pbr.ambient_occlusion;
        // if (material->fbx.ambient_factor.has_value)
        // {
        //     INFO("Ambient Factor Material Map");
        //     scalarMaterialValueShouldCorrespondToValuesInMaterialMap(material->fbx.ambient_factor, actual->pbrMaterial.ambientOcclusionFactor);
        // }

        ufbx_material_map emissionColorMaterialMap = material->fbx.emission_color;
        if (emissionColorMaterialMap.has_value)
        {
            INFO("Emissions Color Material Map");
            float3MaterialValueShouldCorrespondToValuesInMaterialMap(emissionColorMaterialMap, simd_make_float3(
                actual->pbrMaterial.emissionColorAndFactor[0],
                actual->pbrMaterial.emissionColorAndFactor[1],
                actual->pbrMaterial.emissionColorAndFactor[2]));
        }

        ufbx_material_map emissionFactorMaterialMap = material->pbr.emission_factor;
        if (emissionFactorMaterialMap.has_value)
        {
            INFO("Emissions Factor Material Map");
            scalarMaterialValueShouldCorrespondToValuesInMaterialMap(emissionFactorMaterialMap, actual->pbrMaterial.emissionColorAndFactor[3]);
        }
    }
    break;

    default:
        break;
    }
}

void someUfbxMaterialsAreFreed(ufbx_material_list materials)
{
    for (size_t i = 0; i < materials.count; ++i)
    {
        delete materials.data[i];
    }
    delete[] materials.data;
    materials.count = 0;
}

inline void aSceneIsFreed(ufbx_scene* scene)
{
    ufbx_free_scene(scene);
}