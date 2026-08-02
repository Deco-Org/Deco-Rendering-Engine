/**
 * @file material_system_test_fixture.hpp
 * @brief
 */

#pragma once
#include "asset_systems/material_system.hpp"

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
        materialsList[i] = material;
    }

    return materialsList;
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