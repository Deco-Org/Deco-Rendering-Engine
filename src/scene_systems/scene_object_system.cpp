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
    
    number_of_requested_objects += scene_objects.count;
    buffer_manager.add_to_additions_input_buffer(input_buffer_entries, scene_objects.count, largest_handle);
    return handles;
}

void SceneObjectSystem::remove(SceneObjectHandle handle)
{
    remove({
        .data = &handle,
        .count = 1
    });
}

void SceneObjectSystem::remove(SceneObjectHandleList handles)
{
    number_of_requested_objects -= handles.count;
    buffer_manager.add_to_removals_input_buffer(handles.data, handles.count);
}

void SceneObjectSystem::drain_additions_input_buffer()
{
    SceneObjectRenderThreadInputBufferEntry* entries;
    SceneObjectHandle max_handle;
    size_t n;

    buffer_manager.drain_additions_input_buffer(&entries, &n, &max_handle);

    SceneObjectHandle consumed_handles[n];
    if (max_handle >= submesh_handles.size())
    {
        // Allocating space for new entries
        submesh_handles.resize(max_handle + 1);
        transformation_handles.resize(max_handle + 1);
        // animation_instance_handles.resize(max_handle + 1);
        material_handles.resize(max_handle + 1);
        static_sort_key_parts.resize(max_handle + 1);
    }

    for (size_t i = 0; i < n; ++i)
    {
        SceneObjectRenderThreadInputBufferEntry* entry = entries + i;
        consumed_handles[i] = entry->handle;
        submesh_handles[i] = entry->scene_object.submesh_handle;
        transformation_handles[i] = entry->scene_object.transformation_handle;
        // animation_instance_handles[i] = entry->scene_object.animation_instance_handle;
        material_handles[i] = entry->scene_object.material_handle;
        static_sort_key_parts[i] = get_static_sort_key_parts(
            entry->handle,
            entry->scene_object.submesh_handle,
            entry->scene_object.material_handle);
    }

    delete[] entries;
    number_of_objects += n;

    // Filling the output buffer
    buffer_manager.add_to_additions_output_buffer(consumed_handles, n, max_handle);
}

void SceneObjectSystem::drain_removals_input_buffer()
{
    SceneObjectHandle* handles_data;
    size_t number_of_objects_to_remove;
    buffer_manager.drain_removals_input_buffer(&handles_data, &number_of_objects_to_remove);

    for (size_t i = 0; i < number_of_objects_to_remove; ++i)
    {
        SceneObjectHandle handle = handles_data[i];
        submesh_handles[handle] = INVALID_SUBMESH_HANDLE;
        transformation_handles[handle] = TRANSFORMATION_HANDLE_INVALID;
        // animation_instance_handles[handle] = INVALID_ANIMATION_INSTANCE_HANDLE;
        material_handles[handle] = INVALID_MATERIAL;
    }

    number_of_objects -= number_of_objects_to_remove;
}

std::vector<SceneObjectHandle> SceneObjectSystem::get_items_and_drain_output_buffer()
{
    std::vector<SceneObjectHandle> handles;
    SceneObjectHandle* handles_data;
    size_t number_of_handles;
    SceneObjectHandle previous_largest_handle = largest_handle;
    buffer_manager.drain_additions_output_buffer(&handles_data, &number_of_handles, &largest_handle);

    handles.resize(number_of_handles);
    memcpy(handles.data(), handles_data, number_of_handles * sizeof(SceneObjectHandle));
    delete[] handles_data;

    return handles;
}

uint64_t SceneObjectSystem::get_static_sort_key_parts(SceneObjectHandle handle)
{
    MaterialHandle material_handle = material_handles[handle];
    // material_system->material_properties[material_handle] 
    return 0;
}

uint64_t SceneObjectSystem::get_static_sort_key_parts(SceneObjectHandle scene_object_handle, SubmeshHandle submesh_handle, MaterialHandle material_handle)
{
    uint64_t static_sort_key = static_cast<uint64_t>(-1);
    if (material_system)
    {
        static_sort_key = 0;
        MaterialProperties material_properties = material_system->material_properties[material_handle];
        RenderPipelineFlags flags = calculate_pipeline_flags(submesh_handle, material_properties);
    
        static_sort_key |= static_cast<uint64_t>(material_properties) << SortKeyBitMappings::Shifts::Transparency;
        
        if (MaterialProperties::Transparent == (MaterialProperties::Transparent & material_properties))
        {
            static_sort_key |= static_cast<uint64_t>(flags) << static_cast<uint64_t>(SortKeyBitMappings::Shifts::TransparentShifts::PipelineFlags);
        }
        else
        {
            static_sort_key |= static_cast<uint64_t>(flags) << static_cast<uint64_t>(SortKeyBitMappings::Shifts::OpaqueShifts::PipelineFlags);

            uint64_t reduced_material_handle = static_cast<uint64_t>(material_handle) & ((static_cast<uint64_t>(1U) << static_cast<uint64_t>(SortKeyBitMappings::Widths::MaterialHandle)) - 1);
            static_sort_key |= reduced_material_handle << static_cast<uint64_t>(SortKeyBitMappings::Shifts::OpaqueShifts::MaterialHandle);

            uint64_t reduced_submesh_handle = static_cast<uint64_t>(submesh_handle) & ((static_cast<uint64_t>(1U) << static_cast<uint64_t>(SortKeyBitMappings::Widths::SubmeshHandle)) - 1);
            static_sort_key |= reduced_submesh_handle << static_cast<uint64_t>(SortKeyBitMappings::Shifts::OpaqueShifts::SubmeshHandle);
        }
    }
    return static_sort_key;
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

RenderPipelineFlags SceneObjectSystem::calculate_pipeline_flags(
    SubmeshHandle submesh_handle,
    // AnimationInstanceHandle animation_instance_handle,
    MaterialProperties material_properties)
{
    RenderPipelineFlags flags = RenderPipelineFlags::None;
    if (MaterialProperties::Transparent == (MaterialProperties::Transparent & material_properties))
    {
        flags |= RenderPipelineFlags::Translucent;
    }

    if (MaterialProperties::Toon == (MaterialProperties::Toon & material_properties))
    {
        flags |= RenderPipelineFlags::Toon;
    }

    // if (INVALID_ANIMATION_INSTANCE_HANDLE != animation_instance_handle)
    // {
    //     flags |= RenderPipelineFlags::Skinned;
    // }
    return flags;
}