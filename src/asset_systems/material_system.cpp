/**
 * @file material_system.cpp
 * @brief
 */

#include "material_system.hpp"
#include <cstddef>

MaterialSystem::MaterialSystem(TextureLoader* loader)
{
    textureLoader = loader;
    additionsInputBuffer.buffer = nullptr;
    additionsInputBuffer.maxHandle = 0;
    additionsInputBuffer.count = 0;
    additionsOutputBuffer.count = 0;
}

std::vector<MaterialHandle> MaterialSystem::add(MaterialEntryList materials)
{
    std::vector<MaterialHandle> handles;

    MaterialRenderThreadInputBufferEntry inputBufferEntries[materials.count];
    MaterialHandle* handlesToUse = getNextNHandles(materials.count);

    // Filling in the array of input buffer entries
    for (size_t i = 0; i < materials.count; ++i)
    {
        inputBufferEntries[i] = {
            .handle = handlesToUse[i],
            .material = (Material) {
                .type = materials.data[i].type,
                .isTombstone = false,
                (MTL::Texture*)((char*)(&materials.data[i]) + offsetof(MaterialEntry, pbrMaterial))
            }
        };
    }

    // Filling in the handles array while getting the largest handle
    handles.resize(materials.count);
    for (size_t i = 0; i < materials.count; ++i)
    {
        handles[i] = handlesToUse[i];
        if (handles[i] > largestHandle)
            largestHandle = handles[i];
    }
    delete[] handlesToUse;
    
    addInputEntriesToAdditionsBuffer(inputBufferEntries, materials.count);
    return handles;
}

std::vector<MaterialHandle> MaterialSystem::add(ufbx_material_list* materials, MaterialType type)
{
    std::vector<MaterialHandle> handles;

    MaterialRenderThreadInputBufferEntry inputBufferEntries[materials->count];
    MaterialHandle* handlesToUse = getNextNHandles(materials->count);

    // Filling in the array of input buffer entries
    for (size_t i = 0; i < materials->count; ++i)
    {
        ufbx_material* material = materials->data[i];
        Material* loadedMaterial = loadMaterial(material, type);
        inputBufferEntries[i] = {
            .handle = handlesToUse[i],
            .material = *loadedMaterial
        };
    }

    // Filling in the handles array while getting the largest handle
    handles.resize(materials->count);
    for (size_t i = 0; i < materials->count; ++i)
    {
        handles[i] = handlesToUse[i];
        if (handles[i] > largestHandle)
            largestHandle = handles[i];
    }
    delete[] handlesToUse;

    addInputEntriesToAdditionsBuffer(inputBufferEntries, materials->count);
    return handles;
}

void MaterialSystem::remove(MaterialHandleList materials)
{
    const size_t oldSize = freeHandles.size();

    // The list of free handles is not updated here; instead, it is updated when the removals output buffer is drained.
    // This is done to keep the 'semi-freed' slots from being overwritten before the textures can be released on the loading thread.

    // Filling removal buffer
    {
        std::lock_guard<std::mutex> lock(removalsInputBuffer.mutex);

        // Critical section
        const size_t oldBufferSize = removalsInputBuffer.count;
        if (oldBufferSize > 0)
        {
            MaterialHandle* temp = removalsInputBuffer.buffer;
            removalsInputBuffer.buffer = new MaterialHandle[oldBufferSize + materials.count];
            memcpy(removalsInputBuffer.buffer, materials.data, materials.count * sizeof(MaterialHandle));
            memcpy(removalsInputBuffer.buffer + materials.count, temp, oldBufferSize * sizeof(MaterialHandle));
            removalsInputBuffer.count = oldBufferSize + materials.count;
            delete[] temp;
        }
        else
        {
            delete[] removalsInputBuffer.buffer;
            removalsInputBuffer.buffer = new MaterialHandle[materials.count];
            removalsInputBuffer.count = materials.count;
            memcpy(removalsInputBuffer.buffer, materials.data, materials.count * sizeof(MaterialHandle));
        }
    }
}

void MaterialSystem::updateMaterial(MaterialHandle handle, MaterialEntry& material)
{
    MaterialRenderThreadInputBufferEntry updateBufferEntry = {
        .handle = handle,
        .material = {
            .type = material.type,
            .isTombstone = false,
            // .material = material.material
        }
    };

    constexpr size_t materialOffset = sizeof(MaterialEntry) - offsetof(MaterialEntry, material);
    memcpy(&updateBufferEntry.material.material, &material.material, materialOffset);

    size_t count = 1;
    {
        std::lock_guard<std::mutex> lock(updatesInputBuffer.mutex);

        // Critical section
        size_t oldSize = updatesInputBuffer.count;
        if (oldSize > 0)
        {
            // Allocating space for new entries
            const size_t newSize = oldSize + count;
            MaterialRenderThreadInputBufferEntry* temp = updatesInputBuffer.buffer;
            updatesInputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[newSize];
            memcpy(updatesInputBuffer.buffer, temp, oldSize * sizeof(MaterialRenderThreadInputBufferEntry));
            memcpy(updatesInputBuffer.buffer + oldSize, &updateBufferEntry, count * sizeof(MaterialRenderThreadInputBufferEntry));
            updatesInputBuffer.count = newSize;
            delete[] temp;
        }
        else
        {
            if (updatesInputBuffer.buffer)
                delete[] updatesInputBuffer.buffer;
            updatesInputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[count];
            memcpy(updatesInputBuffer.buffer, &updateBufferEntry, count * sizeof(MaterialRenderThreadInputBufferEntry));
            updatesInputBuffer.count = count;
        }
    }
}

void MaterialSystem::updateMaterialTexture(MaterialHandle handle, const MaterialTextureOffset::TextureOffset textureOffset, MTL::Texture* texture)
{
    MaterialRenderThreadUpdateTextureBufferEntry entry = {
        .handle = handle,
        .textureOffset = textureOffset,
        .texture = texture
    };

    size_t count = 1;
    {
        std::lock_guard<std::mutex> lock(textureUpdatesInputBuffer.mutex);

        // Critical section
        size_t oldSize = textureUpdatesInputBuffer.count;
        if (oldSize > 0)
        {
            // Allocating space for new entries
            const size_t newSize = oldSize + count;
            MaterialRenderThreadUpdateTextureBufferEntry* temp = textureUpdatesInputBuffer.buffer;
            textureUpdatesInputBuffer.buffer = new MaterialRenderThreadUpdateTextureBufferEntry[newSize];
            memcpy(textureUpdatesInputBuffer.buffer, temp, oldSize * sizeof(MaterialRenderThreadUpdateTextureBufferEntry));
            memcpy(textureUpdatesInputBuffer.buffer + oldSize, &entry, count * sizeof(MaterialRenderThreadUpdateTextureBufferEntry));
            textureUpdatesInputBuffer.count = newSize;
            delete[] temp;
        }
        else
        {
            if (textureUpdatesInputBuffer.buffer)
                delete[] textureUpdatesInputBuffer.buffer;
            textureUpdatesInputBuffer.buffer = new MaterialRenderThreadUpdateTextureBufferEntry[count];
            memcpy(textureUpdatesInputBuffer.buffer, &entry, count * sizeof(MaterialRenderThreadUpdateTextureBufferEntry));
            textureUpdatesInputBuffer.count = count;
        }
    }
}

void MaterialSystem::drainAdditionsInputBuffer()
{
    MaterialRenderThreadInputBufferEntry* entries;
    MaterialHandle maxHandle;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(additionsInputBuffer.mutex);

        // Critical Section
        n = additionsInputBuffer.count;
        if (n == 0)
            return;
        
        entries = additionsInputBuffer.buffer;
        maxHandle = additionsInputBuffer.maxHandle;
        additionsInputBuffer.buffer = nullptr;
        additionsInputBuffer.count = 0;
    }

    MaterialHandle consumedHandles[n];
    if (maxHandle >= materials.size())
    {
        // Allocating new space for the entries
        materials.reserve(maxHandle + 1);
        const size_t numOfMaterialsToAdd = maxHandle - materials.size();
        for (size_t i = 0; i < numOfMaterialsToAdd + 1; ++i)
        {
            materials.push_back((Material) {
                .type = MaterialType::Unknown,
                .isTombstone = false,
                .pbrMaterial = {0}
            });
        }
    }

    for (size_t i = 0; i < n; ++i)
    {
        MaterialRenderThreadInputBufferEntry* entry = entries + i;
        consumedHandles[i] = entry->handle;
        materials[entry->handle] = entry->material;
    }

    delete[] entries;

    // Filling the output buffer
    {
        std::lock_guard<std::mutex> lock(additionsOutputBuffer.mutex);

        // Critical section
        size_t oldSize = additionsOutputBuffer.count;
        if (oldSize > 0)
        {
            // If there are already items in the output buffer, more space must be allocated
            const size_t newSize = oldSize + n;
            MaterialHandle* temp = additionsOutputBuffer.buffer;
            additionsOutputBuffer.buffer = new MaterialHandle[newSize];
            memcpy(additionsOutputBuffer.buffer, temp, oldSize * sizeof(MaterialHandle));
            memcpy(additionsOutputBuffer.buffer + oldSize, consumedHandles, n * sizeof(MaterialHandle));
            additionsOutputBuffer.count = newSize;
            delete[] temp;
        }
        else
        {
            delete[] additionsOutputBuffer.buffer;
            additionsOutputBuffer.buffer = new MaterialHandle[n];
            memcpy(additionsOutputBuffer.buffer, consumedHandles, n * sizeof(MaterialHandle));
            additionsOutputBuffer.count = n;
        }
        additionsOutputBuffer.largestHandle = materials.size() - 1;
    }
}

std::vector<MaterialHandle> MaterialSystem::getItemsAndDrainAdditionsOutputBuffer()
{
    std::vector<MaterialHandle> consumedHandles;
    {
        std::lock_guard<std::mutex> lock(additionsOutputBuffer.mutex);

        // Critical section
        size_t n = additionsOutputBuffer.count;
        consumedHandles.resize(n);
        memcpy(consumedHandles.data(), additionsOutputBuffer.buffer, n * sizeof(MaterialHandle));
        delete[] additionsOutputBuffer.buffer;
        additionsOutputBuffer.buffer = nullptr;
        additionsOutputBuffer.count = 0;
        largestHandle = additionsOutputBuffer.largestHandle;
    }
    return consumedHandles;
}

void MaterialSystem::drainRemovalsInputBuffer()
{
    MaterialHandle* handlesToRemove;
    MaterialRenderThreadInputBufferEntry* materialsToRemove;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(removalsInputBuffer.mutex);

        // Critical section
        n = removalsInputBuffer.count;
        handlesToRemove = removalsInputBuffer.buffer;
        removalsInputBuffer.buffer = nullptr;
        removalsInputBuffer.count = 0;
    }

    materialsToRemove = new MaterialRenderThreadInputBufferEntry[n]{};
    
    // Materials are designated as tombstones so they are not sent to the GPU
    // The materials are unloaded on the loading thread after the removals thread is drained
    for (size_t i = 0; i < n; ++i)
    {
        materials[handlesToRemove[i]].isTombstone = true;
        materialsToRemove[i] = {
            .handle = handlesToRemove[i],
            .material = materials[handlesToRemove[i]]
        };
    }

    // Loading up the output buffer
    {
        std::lock_guard<std::mutex> lock(removalsOutputBuffer.mutex);

        // Critical section
        size_t oldSize = removalsOutputBuffer.count;
        if (oldSize > 0)
        {
            // If there are already items in the output buffer, more space must be allocated
            const size_t newSize = oldSize + n;
            MaterialRenderThreadInputBufferEntry* temp = removalsOutputBuffer.buffer;
            removalsOutputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[newSize]{};
            memcpy(removalsOutputBuffer.buffer, temp, oldSize * sizeof(MaterialRenderThreadInputBufferEntry));
            memcpy(removalsOutputBuffer.buffer + oldSize, materialsToRemove, n * sizeof(MaterialRenderThreadInputBufferEntry));
            removalsOutputBuffer.count = newSize;
            delete[] temp;
        }
        else
        {
            delete[] removalsOutputBuffer.buffer;
            removalsOutputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[n]{};
            memcpy(removalsOutputBuffer.buffer, materialsToRemove, n * sizeof(MaterialRenderThreadInputBufferEntry));
            removalsOutputBuffer.count = n;
        }
    }

    delete[] handlesToRemove;
    delete[] materialsToRemove;
}

std::vector<MaterialHandle> MaterialSystem::drainRemovalsOutputBufferAndUnloadResources()
{
    std::vector<MaterialHandle> freedMaterialHandles;
    MaterialRenderThreadInputBufferEntry* materialsAndHandlesToFree;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(removalsOutputBuffer.mutex);

        // Critical section
        n = removalsOutputBuffer.count;
        materialsAndHandlesToFree = new MaterialRenderThreadInputBufferEntry[n]{};
        memcpy(materialsAndHandlesToFree, removalsOutputBuffer.buffer, n * sizeof(MaterialRenderThreadInputBufferEntry));
        delete[] removalsOutputBuffer.buffer;
        removalsOutputBuffer.buffer = nullptr;
        removalsOutputBuffer.count = 0;
    }

    freedMaterialHandles.reserve(n);

    // Freeing the materials
    for (size_t i = 0; i < n; ++i)
    {
        Material* material = &materialsAndHandlesToFree[i].material;
        switch (material->type)
        {
            case MaterialType::PBR:
            {
                PBRMaterial* pbr = &material->pbrMaterial;

                if (pbr->albedoTexture)
                    textureLoader->unloadTexture(pbr->albedoTexture);
                if (pbr->normalTexture)
                    textureLoader->unloadTexture(pbr->normalTexture);
                if (pbr->metallicRoughnessAoTexture)
                    textureLoader->unloadTexture(pbr->metallicRoughnessAoTexture);
                if (pbr->emissionTexture)
                    textureLoader->unloadTexture(pbr->emissionTexture);
                
                break;
            }

            case MaterialType::Toon:
            {
                ToonMaterial* toon = &material->toonMaterial;

                if (toon->albedoTexture)
                    textureLoader->unloadTexture(toon->albedoTexture);
                if (toon->shadowThresholdTexture)
                    textureLoader->unloadTexture(toon->shadowThresholdTexture);
                
                break;
            }

            default:
                break;
        }

        // Adding freed handles to freeHandles
        freeHandles.push_back(materialsAndHandlesToFree[i].handle);
    }

    return freedMaterialHandles;
}

void MaterialSystem::drainUpdatesInputBuffers()
{
    std::vector<MTL::Texture*> texturesToUnload = drainMaterialUpdatesInputBufferAndGetTexturesToUnload();
    texturesToUnload.append_range(drainTextureUpdatesInputBufferAndGetTexturesToUnload());

    // Filling in the unload buffer with replaced textures
    {
        std::lock_guard<std::mutex> lock(texturesToUnloadBuffer.mutex);

        // Critical section
        size_t oldSize = texturesToUnloadBuffer.count;
        if (oldSize > 0)
        {
            // Allocating space for new entries
            const size_t newSize = oldSize + texturesToUnload.size();
            MTL::Texture** temp = texturesToUnloadBuffer.buffer;
            texturesToUnloadBuffer.buffer = new MTL::Texture*[newSize];
            memcpy(texturesToUnloadBuffer.buffer, temp, oldSize * sizeof(MTL::Texture*));
            memcpy(texturesToUnloadBuffer.buffer + oldSize, texturesToUnload.data(), texturesToUnload.size() * sizeof(MTL::Texture*));
            texturesToUnloadBuffer.count = newSize;
            delete[] temp;
        }
        else
        {
            if (texturesToUnloadBuffer.buffer)
                delete[] texturesToUnloadBuffer.buffer;
            texturesToUnloadBuffer.buffer = new MTL::Texture*[texturesToUnload.size()];
            memcpy(texturesToUnloadBuffer.buffer, texturesToUnload.data(), texturesToUnload.size() * sizeof(MTL::Texture*));
            texturesToUnloadBuffer.count = texturesToUnload.size();
        }
    }
}

void MaterialSystem::unloadUnusedReplacedTextures()
{
    MTL::Texture** texturesToUnload;
    size_t count;
    {
        std::lock_guard<std::mutex> lock(texturesToUnloadBuffer.mutex);

        // Critical section
        if (texturesToUnloadBuffer.count == 0)
            return;
        
        count = texturesToUnloadBuffer.count;
        texturesToUnload = texturesToUnloadBuffer.buffer;
        texturesToUnloadBuffer.count = 0;
        texturesToUnloadBuffer.buffer = nullptr;
    }

    for (size_t i = 0; i < count; ++i)
    {
        textureLoader->unloadTexture(texturesToUnload[i]);
    }

    delete[] texturesToUnload;
}

Material* MaterialSystem::loadMaterial(ufbx_material* material, MaterialType type)
{
    Material* resultMaterial = new Material{(Material){.type = type, .isTombstone = false, .pbrMaterial = {}}};
    switch (type) 
    {
        case MaterialType::PBR:
        {
            PBRMaterial* pbrMat = &resultMaterial->pbrMaterial;

            // Textures
            // Albedo Color
            if (material->fbx.diffuse_color.has_value)
            {
                if (material->fbx.diffuse_color.texture_enabled && material->fbx.diffuse_color.texture != nullptr)
                {
                    if (material->fbx.diffuse_color.texture->has_file)
                    {
                        std::filesystem::path texturePath = material->fbx.diffuse_color.texture->filename.data;
                        pbrMat->albedoTexture = textureLoader->loadTexture(texturePath);
                    }
                }
                else if (material->fbx.diffuse_color.texture != nullptr && material->fbx.diffuse_color.texture->content.size != 0)
                {
                    // The texture must be loaded from memory
                }
            }
            else
            {
                pbrMat->albedoTexture = nullptr;
            }

            if (material->fbx.normal_map.has_value)
            {
                if (material->fbx.normal_map.texture_enabled && material->fbx.normal_map.texture != nullptr)
                {
                    if (material->fbx.normal_map.texture->has_file)
                    {
                        std::filesystem::path texturePath = material->fbx.normal_map.texture->filename.data;
                        pbrMat->normalTexture = textureLoader->loadTexture(texturePath);
                    } 
                }
            }
            else
            {
                pbrMat->normalTexture = nullptr;
            }

            if (material->fbx.emission_color.has_value)
            {
                if (material->fbx.emission_color.texture_enabled && material->fbx.emission_color.texture != nullptr)
                {
                    if (material->fbx.emission_color.texture->has_file)
                    {
                        std::filesystem::path texturePath = material->fbx.emission_color.texture->filename.data;
                        pbrMat->emissionTexture = textureLoader->loadTexture(texturePath);
                    }
                }
            }
            else
            {
                pbrMat->emissionTexture = nullptr;
            }
            
            // Scalars and Vectors
            pbrMat->baseColorFactor = getFourChannelColorFromUfbxMaterialMap(material->fbx.diffuse_color);
            pbrMat->metallicFactor = getScalarValueFromUfbxMaterialMap(material->fbx.specular_exponent);
            pbrMat->roughnessFactor = getScalarValueFromUfbxMaterialMap(material->fbx.reflection_factor);
            pbrMat->emissionColorAndFactor = simd_make_float4(getThreeChannelColorFromUfbxMaterialMap(material->fbx.emission_color), getScalarValueFromUfbxMaterialMap(material->fbx.emission_factor));
        }
        break;

        default:
        {
            delete resultMaterial;
            resultMaterial = nullptr;
        }
        break;
    }
    return resultMaterial;
}

float MaterialSystem::getScalarValueFromUfbxMaterialMap(ufbx_material_map& materialMap)
{
    float result = 1.0f;
    if (materialMap.has_value)
    {
        switch (materialMap.value_components)
        {
            case 1:
                result = materialMap.value_real;
                break;

            case 2:
                result = (materialMap.value_vec2.x + materialMap.value_vec2.y) / 2;
                break;

            case 3:
                result = (materialMap.value_vec3.x + materialMap.value_vec3.y + materialMap.value_vec3.z) / 3;
                break;

            case 4:
                result = (materialMap.value_vec4.x + materialMap.value_vec4.y + materialMap.value_vec4.z + materialMap.value_vec4.w) / 4;
                break;
        }
    }
    return result;
}

simd_float3 MaterialSystem::getThreeChannelColorFromUfbxMaterialMap(ufbx_material_map& materialMap)
{
    simd_float3 result = DEFAULT_COLOR_3_CHANNELS;
    if (materialMap.has_value)
    {
        switch (materialMap.value_components)
        {
            case 3:
            case 4:
            {
                // If there are three value components, the values are copied.
                // If there are four value components, the alpha is discarded
                result = {
                    materialMap.value_vec3.v[0],
                    materialMap.value_vec3.v[1],
                    materialMap.value_vec3.v[2]
                };
            }
            break;

            case 1:
            {
                result = {
                    materialMap.value_real,
                    materialMap.value_real,
                    materialMap.value_real
                };
            }
            break;

            default:
                break;
        }
    }
    return result;
}

simd_float4 MaterialSystem::getFourChannelColorFromUfbxMaterialMap(ufbx_material_map& materialMap)
{
    simd_float4 result = DEFAULT_COLOR_4_CHANNELS;
    if (materialMap.has_value)
    {
        switch (materialMap.value_components)
        {
            case 4:
            {
                // If there are four values, we can just fill them directly into the result
                result = {
                    materialMap.value_vec4.v[0],
                    materialMap.value_vec4.v[1],
                    materialMap.value_vec4.v[2],
                    materialMap.value_vec4.v[3]
                };
            }
            break;

            case 3:
            {
                // Alpha is just default
                result = {
                    materialMap.value_vec3.v[0],
                    materialMap.value_vec3.v[1],
                    materialMap.value_vec3.v[2],
                    DEFAULT_COLOR_4_CHANNELS[3]
                };
            }
            break;

            case 1:
            {
                result = {
                    materialMap.value_real,
                    materialMap.value_real,
                    materialMap.value_real,
                    DEFAULT_COLOR_4_CHANNELS[3],
                };
            }
            break;

            default:
                break;
        }
    }
    return result;
}

MaterialHandle* MaterialSystem::getNextNHandles(size_t n)
{
    MaterialHandle *handles = new MaterialHandle[n];
    size_t numberOfFreeHandles = freeHandles.size();
    // Getting free handles
    if (n < numberOfFreeHandles)
    {
        memcpy(handles, freeHandles.data(), n * sizeof(MaterialHandle));
        freeHandles.erase(freeHandles.begin(), freeHandles.begin() + n);
    }
    else
    {
        // Take all the freehandles, and put them into the array
        memcpy(handles, freeHandles.data(), freeHandles.size() * sizeof(MaterialHandle));
        freeHandles.resize(0);
        for (size_t i = numberOfFreeHandles; i < n; ++i)
        {
            if (largestHandle == INVALID_MATERIAL)
                largestHandle = 0;
            else
                largestHandle += 1;
            handles[i] = largestHandle;
        }
    }
    return handles;
}

void MaterialSystem::addInputEntriesToAdditionsBuffer(MaterialRenderThreadInputBufferEntry* entries, size_t count)
{
    std::lock_guard<std::mutex> lock(additionsInputBuffer.mutex);

    // Critical section
    size_t oldSize = additionsInputBuffer.count;
    if (oldSize > 0)
    {
        additionsInputBuffer.maxHandle = largestHandle; // largestHandle is updated by both the draining of the free handles and the draining of the output buffer

        // Allocating space for new entries
        const size_t newSize = oldSize + count;
        MaterialRenderThreadInputBufferEntry* temp = additionsInputBuffer.buffer;
        additionsInputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[newSize];
        memcpy(additionsInputBuffer.buffer, temp, oldSize * sizeof(MaterialRenderThreadInputBufferEntry));
        memcpy(additionsInputBuffer.buffer + oldSize, entries, count * sizeof(MaterialRenderThreadInputBufferEntry));
        additionsInputBuffer.count = newSize;
        delete[] temp;
    }
    else
    {
        additionsInputBuffer.maxHandle = largestHandle;
        if (additionsInputBuffer.buffer)
            delete[] additionsInputBuffer.buffer;
        additionsInputBuffer.buffer = new MaterialRenderThreadInputBufferEntry[count];
        memcpy(additionsInputBuffer.buffer, entries, count * sizeof(MaterialRenderThreadInputBufferEntry));
        additionsInputBuffer.count = count;
    }
}

std::vector<MTL::Texture*> MaterialSystem::drainMaterialUpdatesInputBufferAndGetTexturesToUnload()
{
    MaterialRenderThreadInputBufferEntry* entries;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(updatesInputBuffer.mutex);

        // Critical Section
        n = updatesInputBuffer.count;
        if (n == 0)
            return std::vector<MTL::Texture*> {};
        
        entries = updatesInputBuffer.buffer;
        updatesInputBuffer.buffer = nullptr;
        updatesInputBuffer.count = 0;
    }

    std::vector<MTL::Texture*> texturesToUnload;
    texturesToUnload.reserve(n * (uint8_t)MaterialTextureOffset::TextureCounts::MAX_NUMBER_OF_TEXTURES_IN_MATERIAL);

    // Draining the material updates input buffer
    for (size_t i = 0; i < n; ++i)
    {
        MaterialHandle handle = (entries + i)->handle;
        Material* material = &(entries + i)->material;

        // Replacing values and textures
        switch (material->type)
        {
            case (MaterialType::PBR):
            {
                PBRMaterial* pbrMat = &material->pbrMaterial;
                // Looping over the textures in the material being updated. If a texture is being replaced,
                // add the old texture to the unloading buffer.
                for (uint8_t offset = 0; offset < (uint8_t)MaterialTextureOffset::TextureCounts::NUMBER_OF_PBR_TEXTURES; ++offset)
                {
                    MTL::Texture** textureSlot = (MTL::Texture**)(&materials[handle].pbrMaterial) + offset;
                    if (*textureSlot != nullptr)
                        texturesToUnload.push_back(*textureSlot);

                    *textureSlot = *((MTL::Texture**)pbrMat + offset);
                }

                // Copying over the data from the entry to the material
                
                // Getting the offset for the first 'non-texture' field
                constexpr size_t numberOfTextures = (size_t)MaterialTextureOffset::TextureCounts::NUMBER_OF_PBR_TEXTURES;
                constexpr size_t nonTexturesOffset = numberOfTextures * sizeof(MTL::Texture*);

                // The size of the data that comes after the textures. This is likely a bit more data than needed due to 
                // padding, but it shouldn't hurt.
                constexpr size_t nonTextureFieldsSize = sizeof(PBRMaterial) - nonTexturesOffset;

                memcpy(
                    (MTL::Texture**)(&materials[handle].pbrMaterial) + numberOfTextures,
                    (MTL::Texture**)pbrMat + numberOfTextures,
                    nonTextureFieldsSize
                );

                materials[handle].isTombstone = false;
                break;
            }

            case (MaterialType::Toon):
            {
                ToonMaterial* toonMat = &material->toonMaterial;
                for (uint8_t offset = 0; offset < (uint8_t)MaterialTextureOffset::TextureCounts::NUMBER_OF_TOON_TEXTURES; ++offset)
                {
                    MTL::Texture** textureSlot = (MTL::Texture**)toonMat + offset;
                    if (*textureSlot != nullptr)
                        texturesToUnload.push_back(*textureSlot);

                    *textureSlot = *((MTL::Texture**)toonMat + offset);
                }

                // Copying over the data from the entry to the material

                // Getting the offset for the first 'non-texture' field
                constexpr size_t nonTexturesOffset = ((size_t)MaterialTextureOffset::TextureCounts::NUMBER_OF_TOON_TEXTURES * sizeof(MTL::Texture*));
                
                // The size of the data that comes after the textures. This is likely a bit more data than needed due to 
                // padding, but it shouldn't hurt.
                constexpr size_t nonTextureFieldsSize = sizeof(ToonMaterial) - nonTexturesOffset;
                memcpy(
                    (&materials[handle].toonMaterial) + nonTexturesOffset,
                    toonMat + nonTexturesOffset,
                    nonTextureFieldsSize
                );

                materials[handle].isTombstone = false;
                break;
            }

            default:
                break;
        }
    }

    delete[] entries;
    return texturesToUnload;
}

std::vector<MTL::Texture*> MaterialSystem::drainTextureUpdatesInputBufferAndGetTexturesToUnload()
{
    MaterialRenderThreadUpdateTextureBufferEntry* entries;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(textureUpdatesInputBuffer.mutex);

        // Critical Section
        n = textureUpdatesInputBuffer.count;
        if (n == 0)
            return std::vector<MTL::Texture*> {};
        
        entries = textureUpdatesInputBuffer.buffer;
        textureUpdatesInputBuffer.buffer = nullptr;
        textureUpdatesInputBuffer.count = 0;
    }

    std::vector<MTL::Texture*> texturesToUnload;
    texturesToUnload.reserve(n);

    for (size_t i = 0; i < n; ++i)
    {
        MaterialHandle handle = (entries + i)->handle;
        const MaterialTextureOffset::TextureOffset textureOffset = (entries + i)->textureOffset;
        MTL::Texture* texture = (entries + i)->texture;

        MTL::Texture** textureSlot = (((MTL::Texture**)&materials[handle]) + (textureOffset / sizeof(MTL::Texture*)));
        texturesToUnload.push_back(*textureSlot);
        *textureSlot = texture;
    }

    delete[] entries;
    return texturesToUnload;
}
