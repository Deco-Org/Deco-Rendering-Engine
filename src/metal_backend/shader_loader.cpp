/**
 * @file shader_loader.cpp
 * @brief
 */

#include "shader_loader.hpp"
#include "default_metallib.h"

MTL::Library* ShaderLoader::load_shader_library(MTL::Device* device)
{
    dispatch_data_t data = dispatch_data_create(
        default_metallib,
        default_metallib_len,
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), 
        DISPATCH_DATA_DESTRUCTOR_DEFAULT
    );
    MTL::Library* library = device->newLibrary(data, nullptr);

    dispatch_release(data);

    assert(nullptr != library);

    return library;
}