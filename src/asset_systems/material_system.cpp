/**
 * @file material_system.cpp
 * @brief
 */

#include "material_system.hpp"

MaterialSystem::MaterialSystem(TextureLoader* loader)
{
    textureLoader = loader;
}

std::vector<MaterialHandle> MaterialSystem::add(MaterialEntryList materials)
{
    std::vector<MaterialHandle> handles;
    handles.reserve(materials.count);
    for (MaterialHandle handle = 0; handle < materials.count; ++handle)
    {
        handles.push_back(handle);
    }
    return handles;
}

std::vector<MaterialHandle> MaterialSystem::add(ufbx_material_list* materials)
{
    std::vector<MaterialHandle> handles;
    handles.reserve(materials->count);
    for (MaterialHandle handle = 0; handle < materials->count; ++handle)
    {
        handles.push_back(handle);
    }
    return handles;
}

void MaterialSystem::drainAdditionsInputBuffer()
{

}

Material* MaterialSystem::loadMaterial(ufbx_material* material)
{

}