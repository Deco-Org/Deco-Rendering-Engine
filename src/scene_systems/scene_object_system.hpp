/**
 * @file scene_object_system.hpp
 * @brief
 */

#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "asset_systems/submesh_system.hpp"
#include "core_systems/transformation_system.hpp"
#include "asset_systems/material_system.hpp"
#include "metal_backend/render_pipeline_library.hpp"

#pragma once

using SceneObjectHandle = uint32_t;

constexpr SceneObjectHandle INVALID_SCENE_OBJECT_HANDLE = static_cast<SceneObjectHandle>(-1);

struct SceneObject
{
    SubmeshHandle submesh_handle;
    TransformationHandle transformation_handle;
    // AnimationInstanceHandle animation_instance_handle;
    MaterialHandle material_handle;
};

struct SceneObjectRenderThreadInputBufferEntry
{
    SceneObject scene_object;
    SceneObjectHandle handle;
};

struct SceneObjectSystemConfig
{
    SubmeshSystem* submesh_system;
    TransformationSystem* transformation_system;
    // AnimationSystem* animation_system;
    MaterialSystem* material_system;
};

DECO_ENGINE_LIST_TYPE(SceneObjectList, SceneObject);

class SceneObjectSystem
{
public:
    SceneObjectSystem(const SceneObjectSystemConfig& config);
    ~SceneObjectSystem();
    
    SceneObjectHandle add(SceneObject scene_object);
    std::vector<SceneObjectHandle> add(SceneObjectList scene_objects);
    void remove(SceneObjectHandle handle);

    /**
     * Drains the additions input buffer, adds scene objects to the system, and adds consumed handles to the output buffer
     * @warning Should only be called on the render thread.
     */
    void drain_additions_input_buffer();

    /**
     * Drains the removals input buffer and removes scene objects from the system
     * @warning Should only be called on the render thread.
     */
    void drain_removals_input_buffer();

    /**
     * Drains buffer of handles that have been consumed by the render thread.
     * @returns Handles that have been consumed by the render thread.
     * @note This is used to communicate with the render thread.
     */
    std::vector<SceneObjectHandle> get_items_and_drain_output_buffer();

    std::vector<SubmeshHandle> submesh_handles;
    std::vector<TransformationHandle> transformation_handles;
    // std::vector<AnimationInstanceHandle> animation_instance_handles;
    std::vector<MaterialHandle> material_handles;
    std::vector<RenderPipelineBitmap> pipeline_flags;
    std::vector<uint64_t> static_sort_key_parts;
    
    size_t count() const;

private:
    SceneObjectHandle* get_next_n_handles(size_t n);
    void add_input_entries_to_additions_buffer(SceneObjectRenderThreadInputBufferEntry* entries, size_t count);

    SubmeshSystem const* submesh_system;
    TransformationSystem const* transformation_system;
    // AnimationSystem const* animation_system;
    MaterialSystem const* material_system;

    std::vector<SceneObjectHandle> free_handles;

    size_t number_of_objects = 0;
    size_t largest_handle = INVALID_SCENE_OBJECT_HANDLE;

    SystemInputBuffer<SceneObjectRenderThreadInputBufferEntry, SceneObjectHandle> input_entries;
    SynchronizedBuffer<SceneObjectHandle> removal_buffer;
    SystemOutputBuffer<SceneObjectHandle> output_handles;
};
