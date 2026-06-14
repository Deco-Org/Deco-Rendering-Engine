#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void rendering_engine_init(void* mtlLayer, int width, int height);
void rendering_engine_shutdown();
void rendering_engine_draw(void* drawable);

#ifdef __cplusplus
}
#endif