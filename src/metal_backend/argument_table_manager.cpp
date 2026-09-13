/**
 * @file argument_table_manager.cpp
 * @brief
 */

#include "argument_table_manager.hpp"

ArgumentTableManager::ArgumentTableManager(MTL::Device* metal_device)
{
    device = metal_device;
    MTL4::ArgumentTableDescriptor* argument_table_descriptor = MTL4::ArgumentTableDescriptor::alloc()->init();
    MTL4::ArgumentTable* argument_table = device->newArgumentTable(argument_table_descriptor, nullptr);

    create_vertex_argument_table(MaterialType::PBR);
    create_fragment_argument_table(MaterialType::PBR);
}

void ArgumentTableManager::bind_buffer(MTL::Buffer* buffer, RenderingArgumentSlot argument_slot, ShaderType shader_type)
{
    MTL4::ArgumentTable* argument_table = get_argument_table_from_shader_type(shader_type);
    argument_table->setAddress(buffer->gpuAddress(), argument_slot);
}

void ArgumentTableManager::bind_texture(MTL::Texture* texture, RenderingArgumentSlot argument_slot, ShaderType shader_type)
{
    MTL4::ArgumentTable* argument_table = get_argument_table_from_shader_type(shader_type);
    argument_table->setTexture(texture->gpuResourceID(), argument_slot);
}
    
void ArgumentTableManager::bind_sampler(MTL::SamplerState* sampler_state, RenderingArgumentSlot argument_slot, ShaderType shader_type)
{
    MTL4::ArgumentTable* argument_table = get_argument_table_from_shader_type(shader_type);
    argument_table->setSamplerState(sampler_state->gpuResourceID(), argument_slot);
}

constexpr MTL4::ArgumentTable* ArgumentTableManager::get_argument_table_from_shader_type(ShaderType shader_type) const
{
    MTL4::ArgumentTable* argument_table;
    switch (shader_type)
    {
        case ShaderType::PBRVertex:
        case ShaderType::ToonVertex:
            argument_table = vertex_argument_table;
            break;

        case ShaderType::PBRFragment:
        case ShaderType::ToonFragment:
            argument_table = fragment_argument_table;
            break;

        default:
            argument_table = nullptr;
            break;
    };

    return argument_table;
}

void ArgumentTableManager::create_vertex_argument_table(MaterialType material_type)
{
    NS::Error* error = nullptr;

    MTL4::ArgumentTableDescriptor* argument_table_descriptor;
    argument_table_descriptor = MTL4::ArgumentTableDescriptor::alloc()->init();

    switch (material_type)
    {
        case MaterialType::PBR:
            argument_table_descriptor->setMaxBufferBindCount(static_cast<NS::UInteger>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_VERTEX_BUFFERS));
            break;

        default:
            break;
    }

    vertex_argument_table = device->newArgumentTable(argument_table_descriptor, &error);

    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    argument_table_descriptor->release();
}

void ArgumentTableManager::create_fragment_argument_table(MaterialType material_type)
{
    NS::Error* error = nullptr;

    MTL4::ArgumentTableDescriptor* argument_table_descriptor;
    argument_table_descriptor = MTL4::ArgumentTableDescriptor::alloc()->init();

    switch (material_type)
    {
        case MaterialType::PBR:
        {
            argument_table_descriptor->setMaxBufferBindCount(static_cast<NS::UInteger>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_BUFFERS));
            argument_table_descriptor->setMaxSamplerStateBindCount(static_cast<NS::UInteger>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_SAMPLERS));
            argument_table_descriptor->setMaxTextureBindCount(static_cast<NS::UInteger>(ArgumentSlots::RenderingSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_TEXTURES));
            break;
        }

        default:
        break;
    }

    fragment_argument_table = device->newArgumentTable(argument_table_descriptor, &error);

    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    argument_table_descriptor->release();
}