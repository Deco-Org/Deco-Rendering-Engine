#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum {
    RenderableId_INVALID = -1,
};
typedef uint16_t RenderableHandle_t;

void rendering_engine_init(void* mtlLayer, int width, int height);
void rendering_engine_shutdown();
void rendering_engine_draw(void* drawable);

/**
 * Load a model into memory
 * @param path The file path to the model
 * @returns An ID of the 
 */
const RenderableHandle_t rendering_engine_load_model(const char* path);

/**
 * Unloads a model from memory.
 * @param renderable The ID of the model to unload
 */
void rendering_engine_unload_model(RenderableHandle_t handle);

#ifdef __cplusplus
}
#endif