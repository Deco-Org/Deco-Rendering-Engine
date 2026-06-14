#include "rendering_engine_api.h"
// #include <QuartzCore/QuartzCore.hpp>
#include "rendering_engine.hpp"
#include <stdio.h>

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

static RenderingEngine *engine = nullptr;

void rendering_engine_init(void* mtlLayer, int width, int height)
{
    CA::MetalLayer *layer = reinterpret_cast<CA::MetalLayer*>(mtlLayer);
    engine = new RenderingEngine();
    engine->init(layer, width, height);
}

void rendering_engine_shutdown()
{
    engine->cleanup();
    delete engine;
    engine = nullptr;
}

void rendering_engine_draw(void *drawablePtr)
{
    CA::MetalDrawable *drawable = reinterpret_cast<CA::MetalDrawable*>(drawablePtr);
    engine->draw(drawable);
}

#ifdef __cplusplus
}
#endif
