/**
 * @file submesh_system.cpp
 * @brief
 */

#include "submesh_system.hpp"

SubmeshSystem::SubmeshSystem(MTL::Device* metalDevice)
{
    device = metalDevice;

    std::lock_guard<std::mutex> lock(inputEntries.mutex);
    // Critical section
    inputEntries.buffer = nullptr;
    inputEntries.count = 0;
    inputEntries.maxHandle = 0;
}

SubmeshSystem::~SubmeshSystem()
{
    // Releasing all buffers
    for (SubmeshHandle handle = 0; handle < largestHandle; ++handle)
    {
        vertexBuffers[handle]->release();
        indexBuffers[handle]->release();
    }
}

std::vector<SubmeshHandle> SubmeshSystem::add(ufbx_mesh* mesh)
{
    std::vector<SubmeshHandle> handles;

    // Getting the submeshes (the parts of the mesh that use different materials)
    ufbx_mesh_part_list submeshes = mesh->material_parts;
    size_t numberOfSubmeshes = submeshes.count;
    SubmeshRenderThreadInputBufferEntry inputBufferEntries[numberOfSubmeshes];
    SubmeshHandle* handlesToUse = getNextNHandles(numberOfSubmeshes);
    for (size_t i = 0; i < numberOfSubmeshes; ++i)
    {
        ufbx_mesh_part* submesh = &submeshes.data[i];
        inputBufferEntries[i] = generateInputEntryForSubmesh(mesh, submesh, handlesToUse[i]);
    }
    
    // Filling in the handles array while getting the largest handle
    handles.resize(numberOfSubmeshes);
    for (size_t i = 0; i < numberOfSubmeshes; ++i)
    {
        handles[i] = handlesToUse[i];
        if (handles[i] > largestHandle)
            largestHandle = handles[i];
    }
    delete[] handlesToUse;

    addInputEntriesToAdditionsBuffer(inputBufferEntries, numberOfSubmeshes);
    return handles;
}

SubmeshHandle SubmeshSystem::add(ufbx_mesh* mesh, ufbx_mesh_part* submesh)
{
    SubmeshHandle* handles = getNextNHandles(1);
    SubmeshHandle handle = handles[0];
    delete[] handles;

    SubmeshRenderThreadInputBufferEntry inputBufferEntry = generateInputEntryForSubmesh(mesh, submesh, handle);
    addInputEntriesToAdditionsBuffer(&inputBufferEntry, 1);
    return handle;
}

void SubmeshSystem::remove(SubmeshList submeshes)
{
    const size_t oldSize = freeHandles.size();

    // Adding to list of free handles
    freeHandles.resize(oldSize + submeshes.count);
    memcpy(freeHandles.data() + oldSize, submeshes.data, submeshes.count * sizeof(SubmeshHandle));

    // Filling removal buffer
    {
        std::lock_guard<std::mutex> lock(removalBuffer.mutex);

        // Critical section
        const size_t oldBufferSize = removalBuffer.count;
        if (oldBufferSize > 0)
        {
            SubmeshHandle* temp = removalBuffer.buffer;
            removalBuffer.buffer = new SubmeshHandle[oldBufferSize + submeshes.count];
            memcpy(removalBuffer.buffer, submeshes.data, submeshes.count * sizeof(SubmeshHandle));
            memcpy(removalBuffer.buffer + submeshes.count, temp, oldBufferSize * sizeof(SubmeshHandle));
            removalBuffer.count = oldBufferSize + submeshes.count;
            delete[] temp;
        }
        else
        {
            delete[] removalBuffer.buffer;
            removalBuffer.buffer = new SubmeshHandle[submeshes.count];
            removalBuffer.count = submeshes.count;
            memcpy(removalBuffer.buffer, submeshes.data, submeshes.count * sizeof(SubmeshHandle));
        }
    }
}

void SubmeshSystem::drainAdditionsInputBuffer()
{
    SubmeshRenderThreadInputBufferEntry* entries;
    SubmeshHandle maxHandle;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(inputEntries.mutex);
        
        // Critical section
        n = inputEntries.count;
        if (n == 0)
            return;
        
        entries = inputEntries.buffer;
        maxHandle = inputEntries.maxHandle;
        inputEntries.buffer = nullptr;
        inputEntries.count = 0;
    }

    SubmeshHandle consumedHandles[n];
    if (maxHandle >= vertexBuffers.size())
    {
        // Allocating new space for the entries
        vertexBuffers.resize(maxHandle + 1);
        indexBuffers.resize(maxHandle + 1);
        indexCounts.resize(maxHandle + 1);
        boundsMin.resize(maxHandle + 1);
        boundsMax.resize(maxHandle + 1);
        skinningProperties.resize(maxHandle + 1);
        boneCounts.resize(maxHandle + 1);
    }
    
    for (size_t i = 0; i < n; ++i)
    {
        SubmeshRenderThreadInputBufferEntry* entry = entries + i;
        consumedHandles[i] = entry->handle;
        vertexBuffers[entry->handle] = entry->vertexBuffer;
        indexBuffers[entry->handle] = entry->indexBuffer;
        indexCounts[entry->handle] = entry->indexCount;
        boundsMin[entry->handle] = entry->boundsMin;
        boundsMax[entry->handle] = entry->boundsMax;
        skinningProperties[entry->handle] = entry->skinningProperty;
        boneCounts[entry->handle] = entry->boneCount;
    }

    delete[] entries;

    // Filling output buffer
    {
        std::lock_guard<std::mutex> lock(outputHandles.mutex);
        
        // Critical section
        size_t oldSize = outputHandles.count;
        if (oldSize > 0)
        {
            // If there are already items in the output buffer, more space must be allocated
            const size_t newSize = oldSize + n;
            SubmeshHandle* temp = outputHandles.buffer;
            outputHandles.buffer = new SubmeshHandle[newSize];
            memcpy(outputHandles.buffer, temp, oldSize * sizeof(SubmeshHandle));
            memcpy(outputHandles.buffer + oldSize, consumedHandles, n * sizeof(SubmeshHandle));
            outputHandles.count = newSize;
            delete[] temp;
        }
        else
        {
            delete[] outputHandles.buffer;
            outputHandles.buffer = new SubmeshHandle[n];
            memcpy(outputHandles.buffer, consumedHandles, n * sizeof(SubmeshHandle));
            outputHandles.count = n;
        }
        outputHandles.largestHandle = vertexBuffers.size() - 1;
    }
}

void SubmeshSystem::drainRemovalBuffer()
{
    SubmeshHandle* handlesToRemove;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(removalBuffer.mutex);

        // Critical section
        n = removalBuffer.count;
        handlesToRemove = removalBuffer.buffer;
        removalBuffer.buffer = nullptr;
        removalBuffer.count = 0;
    }
    
    for (size_t i = 0; i < n; ++i)
    {
        indexCounts[handlesToRemove[i]] = INVALID_INDEX_COUNT;
        
        if (vertexBuffers[handlesToRemove[i]])
        {
            vertexBuffers[handlesToRemove[i]]->release();
            vertexBuffers[handlesToRemove[i]] = nullptr;
        }
        if (indexBuffers[handlesToRemove[i]])
        {
            indexBuffers[handlesToRemove[i]]->release();
            indexBuffers[handlesToRemove[i]] = nullptr;
        }
    }

    delete[] handlesToRemove;
}

std::vector<SubmeshHandle> SubmeshSystem::getItemsAndDrainOutputBuffer()
{
    std::vector<SubmeshHandle> consumedHandles;
    {
        std::lock_guard<std::mutex> lock(outputHandles.mutex);
        
        // Critical section
        size_t n = outputHandles.count;
        consumedHandles.resize(n);
        memcpy(consumedHandles.data(), outputHandles.buffer, n * sizeof(SubmeshHandle));
        delete[] outputHandles.buffer;
        outputHandles.buffer = nullptr;
        outputHandles.count = 0;
        largestHandle = outputHandles.largestHandle;
    }
    return consumedHandles;
}

SubmeshRenderThreadInputBufferEntry SubmeshSystem::generateInputEntryForSubmesh(
    ufbx_mesh* mesh,
    ufbx_mesh_part* submesh,
    SubmeshHandle handle)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> triIndices;
    triIndices.resize(mesh->max_face_triangles * 3);

    // Iterate over each face
    for (uint32_t faceIndex : submesh->face_indices)
    {
        ufbx_face face = mesh->faces[faceIndex];

        // Triangulating the face into `triIndices`
        uint32_t numberOfTriangles = ufbx_triangulate_face(
            triIndices.data(), triIndices.size(), mesh, face);
        
        // Iterate over each triangle corner contiguously.
        for (size_t i = 0; i < numberOfTriangles * 3; ++i)
        {
            uint32_t index = triIndices[i];
            Vertex v;
            v.position = {
                mesh->vertex_position[index].x,
                mesh->vertex_position[index].y,
                mesh->vertex_position[index].z
            };
            v.normal = {
                mesh->vertex_normal[index].x,
                mesh->vertex_normal[index].y,
                mesh->vertex_normal[index].z
            };
            v.uv = {
                mesh->vertex_uv[index].x,
                mesh->vertex_uv[index].y
            };
            vertices.push_back(v);
        }
    }

    assert(vertices.size() == submesh->num_triangles * 3);

    // Generating the index buffer
    ufbx_vertex_stream streams[1] = {
        { vertices.data(), vertices.size(), sizeof(Vertex) }
    };
    std::vector<uint32_t> indices;
    indices.resize(submesh->num_triangles * 3);

    // This call deduplicates vertices, modifying the arrays passed in `streams[]`,
    // writing indices into `indices[]`, and returning the number of unique vertices.
    size_t numberOfVertices = ufbx_generate_indices(
        streams,
        1,
        indices.data(),
        indices.size(),
        nullptr,
        nullptr
    );

    vertices.resize(numberOfVertices);

    // Getting the n next available handles

    SubmeshRenderThreadInputBufferEntry inputBufferEntry;
    inputBufferEntry.handle = handle;
    createAndFillVertexAndIndexBuffers(
        inputBufferEntry,
        vertices,
        indices);
    inputBufferEntry.indexCount = indices.size();
    inputBufferEntry.skinningProperty = (mesh->skin_deformers.count > 0) ? SubmeshSkinningProperty::Skinned : SubmeshSkinningProperty::Unskinned;

    return inputBufferEntry;
}

void SubmeshSystem::addInputEntriesToAdditionsBuffer(SubmeshRenderThreadInputBufferEntry* entries, size_t count)
{
    std::lock_guard<std::mutex> lock(inputEntries.mutex);

    // Critical section
    size_t oldSize = inputEntries.count;
    if (oldSize > 0)
    {
        inputEntries.maxHandle = largestHandle; // largestHandle is updated by both the draining of the free handles and the draining of the output buffer

        // Allocating space for new entries
        const size_t newSize = oldSize + count;
        SubmeshRenderThreadInputBufferEntry* temp = inputEntries.buffer;
        inputEntries.buffer = new SubmeshRenderThreadInputBufferEntry[newSize];
        memcpy(inputEntries.buffer, temp, oldSize * sizeof(SubmeshRenderThreadInputBufferEntry));
        memcpy(inputEntries.buffer + oldSize, entries, count * sizeof(SubmeshRenderThreadInputBufferEntry));
        inputEntries.count = newSize;
        delete[] temp;
    }
    else
    {
        inputEntries.maxHandle = largestHandle;
        if (inputEntries.buffer)
            delete[] inputEntries.buffer;
            inputEntries.buffer = new SubmeshRenderThreadInputBufferEntry[count];
        memcpy(inputEntries.buffer, entries, count * sizeof(SubmeshRenderThreadInputBufferEntry));
        inputEntries.count = count;
    }
}

void SubmeshSystem::createAndFillVertexAndIndexBuffers(
    SubmeshRenderThreadInputBufferEntry& entry,
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices)
{
    if (vertices.size() == 0 || indices.size() == 0)
        return;

    if (device == nullptr)
    {
        entry.vertexBuffer = nullptr;
        entry.indexBuffer = nullptr;
    }
    else
    {
        entry.vertexBuffer = device->newBuffer(vertices.data(), vertices.size() * sizeof(Vertex), MTL::ResourceStorageModeShared);
        entry.indexBuffer = device->newBuffer(indices.data(), indices.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared);
    }
}

SubmeshHandle* SubmeshSystem::getNextNHandles(size_t n)
{
    SubmeshHandle *handles = new SubmeshHandle[n];
    size_t numberOfFreeHandles = freeHandles.size();
    // Getting free handles
    if (n < numberOfFreeHandles)
    {
        memcpy(handles, freeHandles.data(), n * sizeof(SubmeshHandle));
        freeHandles.erase(freeHandles.begin(), freeHandles.begin() + n);
    }
    else
    {
        // Take all the freehandles, and put them into the array
        memcpy(handles, freeHandles.data(), freeHandles.size() * sizeof(SubmeshHandle));
        freeHandles.resize(0);
        for (size_t i = numberOfFreeHandles; i < n; ++i)
        {
            if (largestHandle == INVALID_SUBMESH_HANDLE)
                largestHandle = 0;
            else
                largestHandle += 1;
            handles[i] = largestHandle;
        }
    }
    return handles;
}
