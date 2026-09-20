/**
 * @file scene_object_system.cpp
 * @brief
 */

#include "scene_object_system.hpp"

SceneObjectSystem::SceneObjectSystem(const SceneObjectSystemConfig& config)
{
    submesh_system = config.submesh_system;
    transformation_system = config.transformation_system;
    // animation_system = config.animation_system;
    material_system = config.material_system;
}

SceneObjectSystem::~SceneObjectSystem()
{

}

SceneObjectHandle SceneObjectSystem::add(SceneObject scene_object)
{
    SceneObjectHandle handle = add((SceneObjectList){
        .data = &scene_object,
        .count = 1,
    })[0];
    return handle;
}

std::vector<SceneObjectHandle> SceneObjectSystem::add(SceneObjectList scene_objects)
{
    std::vector<SceneObjectHandle> handles;

    SceneObjectRenderThreadInputBufferEntry input_buffer_entries[scene_objects.count];
    SceneObjectHandle* handles_to_use = get_next_n_handles(scene_objects.count);

    for (size_t i = 0; i < scene_objects.count; ++i)
    {
        input_buffer_entries[i].scene_object = scene_objects[i];
        input_buffer_entries[i].handle = handles_to_use[i];
    }

    // Filling in the handles array while getting the largest handle
    handles.resize(scene_objects.count);
    for (size_t i = 0; i < scene_objects.count; ++i)
    {
        handles[i] = handles_to_use[i];
        if (handles[i] > largest_handle)
            largest_handle = handles[i];
    }
    
    delete[] handles_to_use;

    


}

void SceneObjectSystem::drain_additions_input_buffer()
{
    SceneObjectRenderThreadInputBufferEntry* entries;
    SceneObjectHandle max_handle;
    size_t n;

    {
        std::lock_guard<std::mutex> lock(input_entries.mutex);

        // Critical section
        n = input_entries.count;
        if (n == 0)
            return;

        entries = input_entries.buffer;
        max_handle = input_entries.maxHandle;
        input_entries.buffer = nullptr;
        input_entries.count = 0;
        input_entries.maxHandle = INVALID_SCENE_OBJECT_HANDLE;
    }

    SceneObjectHandle consumed_handles[n];
    if (max_handle >= submesh_handles.size())
    {
        // Allocating space for new entries
        submesh_handles.resize(max_handle + 1);
        transformation_handles.resize(max_handle + 1);
        // animation_instance_handles.resize(max_handle + 1);
        material_handles.resize(max_handle + 1);
    }

    for (size_t i = 0; i < n; ++i)
    {
        SceneObjectRenderThreadInputBufferEntry* entry = entries + i;
        consumed_handles[i] = entry->handle;
        submesh_handles[i] = entry->scene_object.submesh_handle;
        transformation_handles[i] = entry->scene_object.transformation_handle;
        // animation_instance_handles[i] = entry->scene_object.animation_instance_handle;
        material_handles[i] = entry->scene_object.material_handle;
    }

    delete[] entries;

    // Filling the output buffer
    {
        std::lock_guard<std::mutex> lock(output_handles.mutex);

        // Critical section
        size_t old_size = output_handles.count;
        if (old_size > 0)
        {
            const size_t new_size = old_size + n;
            SceneObjectHandle* temp = output_handles.buffer;
            output_handles.buffer = new SceneObjectHandle[new_size];
            memcpy(output_handles.buffer, temp, old_size * sizeof(SceneObjectHandle));
            memcpy(output_handles.buffer + old_size, consumed_handles, n * sizeof(SceneObjectHandle));
            output_handles.count = new_size;
            delete[] temp;
        }
        else
        {
            delete[] output_handles.buffer;
            output_handles.buffer = new SubmeshHandle[n];
            memcpy(output_handles.buffer, consumed_handles, n * sizeof(SceneObjectHandle));
            output_handles.count = n;
        }
        output_handles.largestHandle = submesh_handles.size();
    }
}

size_t SceneObjectSystem::count() const
{
    return number_of_objects;
}

SceneObjectHandle* SceneObjectSystem::get_next_n_handles(size_t n)
{
    SceneObjectHandle* handles = new SceneObjectHandle[n];
    size_t number_of_free_handles = free_handles.size();
    // Getting free handles
    if (n < number_of_free_handles)
    {
        memcpy(handles, free_handles.data(), n * sizeof(SceneObjectHandle));
        free_handles.erase(free_handles.begin(), free_handles.begin() + n);
    }
    else
    {
        // Take all the free handles, and put them into the array
        memcpy(handles, free_handles.data(), free_handles.size() * sizeof(SceneObjectHandle));
        free_handles.resize(0);
        for (size_t i = number_of_free_handles; i < n; ++i)
        {
            if (largest_handle == INVALID_SCENE_OBJECT_HANDLE)
                largest_handle = 0;
            else
                largest_handle += 1;
            handles[i] = largest_handle;
        }
    }
    return handles;
}

void SceneObjectSystem::add_input_entries_to_additions_buffer(SceneObjectRenderThreadInputBufferEntry* entries, size_t count) {};