/**
 * @file submesh_system.cpp
 * @brief
 */

#include "submesh_system.hpp"

SubmeshSystem::SubmeshSystem(MTL::Device* metalDevice)
{
    device = metalDevice;

    std::lock_guard<std::mutex> lock(buffer_manager.additions_input.mutex);
    // Critical section
    buffer_manager.additions_input.buffer = nullptr;
    buffer_manager.additions_input.count = 0;
    buffer_manager.additions_input.max_handle = 0;
}

SubmeshSystem::~SubmeshSystem()
{
    // Releasing all buffers
    if (largestHandle != INVALID_SUBMESH_HANDLE)
    {
        for (SubmeshHandle handle = 0; handle < largestHandle; ++handle)
        {
            if (vertexBuffers[handle])
                vertexBuffers[handle]->release();
            if (indexBuffers[handle])
                indexBuffers[handle]->release();
        }
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
    buffer_manager.add_to_removals_input_buffer(submeshes.data, submeshes.count);
}

void SubmeshSystem::drainAdditionsInputBuffer()
{
    SubmeshRenderThreadInputBufferEntry* entries;
    SubmeshHandle max_handle;
    size_t n;

    buffer_manager.drain_additions_input_buffer(&entries, &n, &max_handle);

    SubmeshHandle consumedHandles[n];
    if (max_handle >= vertexBuffers.size())
    {
        // Allocating new space for the entries
        vertexBuffers.resize(max_handle + 1);
        indexBuffers.resize(max_handle + 1);
        indexCounts.resize(max_handle + 1);
        boundsMin.resize(max_handle + 1);
        boundsMax.resize(max_handle + 1);
        skinningProperties.resize(max_handle + 1);
        boneCounts.resize(max_handle + 1);
    }
    
    for (size_t i = 0; i < n; ++i)
    {
        SubmeshRenderThreadInputBufferEntry* entry = entries + i;
        consumedHandles[i] = entry->handle;
        vertexBuffers[entry->handle] = entry->entry.vertexBuffer;
        indexBuffers[entry->handle] = entry->entry.indexBuffer;
        indexCounts[entry->handle] = entry->entry.indexCount;
        boundsMin[entry->handle] = entry->entry.boundsMin;
        boundsMax[entry->handle] = entry->entry.boundsMax;
        skinningProperties[entry->handle] = entry->entry.skinningProperty;
        boneCounts[entry->handle] = entry->entry.boneCount;
    }

    delete[] entries;

    // Filling output buffer
    buffer_manager.add_to_additions_output_buffer(consumedHandles, n, vertexBuffers.size() - 1);
}

void SubmeshSystem::drainRemovalBuffer()
{
    SubmeshHandle* handlesToRemove;
    size_t n;

    buffer_manager.drain_removals_input_buffer(&handlesToRemove, &n);
    
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
    std::vector<SubmeshHandle> consumed_handles;
    buffer_manager.drain_additions_output_buffer_into_resizable_container(consumed_handles, largestHandle);
    return consumed_handles;
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
    inputBufferEntry.entry.indexCount = indices.size();
    inputBufferEntry.entry.skinningProperty = (mesh->skin_deformers.count > 0) ? SubmeshSkinningProperty::Skinned : SubmeshSkinningProperty::Unskinned;

    return inputBufferEntry;
}

void SubmeshSystem::addInputEntriesToAdditionsBuffer(SubmeshRenderThreadInputBufferEntry* entries, size_t count)
{
    buffer_manager.add_to_additions_input_buffer(
        entries,
        count,
        largestHandle
    );
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
        entry.entry.vertexBuffer = nullptr;
        entry.entry.indexBuffer = nullptr;
    }
    else
    {
        entry.entry.vertexBuffer = device->newBuffer(vertices.data(), vertices.size() * sizeof(Vertex), MTL::ResourceStorageModeShared);
        entry.entry.indexBuffer = device->newBuffer(indices.data(), indices.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared);
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
