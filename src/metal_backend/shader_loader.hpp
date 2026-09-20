/**
 * @file shader_loader.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>

class ShaderLoader
{
    public:

    static MTL::Library* load_shader_library(MTL::Device* device);
};