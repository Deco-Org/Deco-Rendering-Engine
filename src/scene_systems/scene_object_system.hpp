/**
 * @file scene_object_system.hpp
 * @brief
 */

#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "tools/thread_communication.hpp"
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
DECO_ENGINE_LIST_TYPE(SceneObjectHandleList, SceneObjectHandle);

class SceneObjectSystem
{
public:
    SceneObjectSystem(const SceneObjectSystemConfig& config);
    ~SceneObjectSystem();
    
    /**
     * Add a scene object to the system.
     * @param scene_object The object to be added.
     * @returns The handle of the added object.
     */
    SceneObjectHandle add(SceneObject scene_object);

    /**
     * Adds multiple scene objects to the system.
     * @param scene_objects A list of scene objects to be added.
     * @returns The handles of the added objects.
     */
    std::vector<SceneObjectHandle> add(SceneObjectList scene_objects);

    /**
     * Request an object's removal from the system.
     * @param handle The handle of the object to be removed.
     */
    void remove(SceneObjectHandle handle);

    /**
     * Request the removal of multiple objects from the system.
     * @param handles The handles of the objects to be removed.
     */
    void remove(SceneObjectHandleList handles);

    /**
     * Drains the additions input buffer, adds scene objects to the system, and adds consumed handles to the output buffer.
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
    
    /**
     * @returns The number of scene objects in the system
     * @warning Should only be called on the render thread.
     */
    size_t count() const;

    std::vector<SubmeshHandle> submesh_handles;
    std::vector<TransformationHandle> transformation_handles;
    // std::vector<AnimationInstanceHandle> animation_instance_handles;
    std::vector<MaterialHandle> material_handles;
    std::vector<RenderPipelineBitmap> pipeline_flags;
    std::vector<uint64_t> static_sort_key_parts;

private:
    SceneObjectHandle* get_next_n_handles(size_t n);

    SubmeshSystem const* submesh_system;
    TransformationSystem const* transformation_system;
    // AnimationSystem const* animation_system;
    MaterialSystem const* material_system;

    std::vector<SceneObjectHandle> free_handles;

    size_t number_of_objects = 0;
    size_t number_of_requested_objects = 0;
    SceneObjectHandle largest_handle = INVALID_SCENE_OBJECT_HANDLE;

    ThreadCommunication::BufferManager<SceneObject, SceneObjectHandle, SceneObjectRenderThreadInputBufferEntry> buffer_manager;
};
