/**
 * @file scene_object_system.hpp
 * @brief
 */

#include <Metal/Metal.hpp>
#include "core_engine_types.h"
#include "submesh_system.hpp"
#include "transformation_system.hpp"
#include "material_system.hpp"
#include "render_pipeline_library.hpp"

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

struct SceneObjectSystemConfig
{
    SubmeshSystem* submesh_system;
    TransformationSystem* transformation_system;
    // AnimationSystem* animation_system;
    MaterialSystem* material_system;
};

class SceneObjectSystem
{
public:
    SceneObjectSystem(const SceneObjectSystemConfig& config);
    ~SceneObjectSystem();
    
    SceneObjectHandle add(SceneObject scene_object);
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
    SubmeshSystem* submesh_system;
    TransformationSystem* transformation_system;
    // AnimationSystem* animation_system;
    MaterialSystem* material_system;

    SystemInputBuffer<SceneObject, SceneObjectHandle> input_entries;
    SynchronizedBuffer<SceneObjectHandle> removal_buffer;
    SystemOutputBuffer<SceneObjectHandle> output_handles;
};
