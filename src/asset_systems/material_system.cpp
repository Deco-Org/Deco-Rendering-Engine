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

std::vector<MaterialHandle> MaterialSystem::add(ufbx_material_list* materials)
{
    std::vector<MaterialHandle> handles;

    MaterialRenderThreadInputBufferEntry inputBufferEntries[materials->count];
    MaterialHandle* handlesToUse = getNextNHandles(materials->count);

    // Filling in the array of input buffer entries
    for (size_t i = 0; i < materials->count; ++i)
    {
        ufbx_material* material = materials->data[i];
        Material* loadedMaterial = loadMaterial(material);
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

}

Material* MaterialSystem::loadMaterial(ufbx_material* material)
{

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