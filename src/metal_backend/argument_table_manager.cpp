/**
 * @file argument_table_manager.cpp
 * @brief
 */

#include "argument_table_manager.hpp"

ArgumentTableManager::ArgumentTableManager(MTL::Device* metal_device)
{
    device = metal_device;

    create_argument_table(ShaderType::VertexShader);
    create_argument_table(ShaderType::PBRFragment);
    create_argument_table(ShaderType::ToonFragment);
}

ArgumentTableManager::~ArgumentTableManager()
{
    vertex_argument_table->release();
    pbr_fragment_argument_table->release();
    toon_fragment_argument_table->release();
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

void ArgumentTableManager::apply_tables(MTL4::RenderCommandEncoder* encoder, MaterialType material_type)
{
    encoder->setArgumentTable(vertex_argument_table, MTL::RenderStageVertex);

    switch (material_type)
    {
        case MaterialType::PBR:
            encoder->setArgumentTable(pbr_fragment_argument_table, MTL::RenderStageFragment);
            break;

        case MaterialType::Toon:
            encoder->setArgumentTable(toon_fragment_argument_table, MTL::RenderStageFragment);
            break;
    }
}

constexpr MTL4::ArgumentTable* ArgumentTableManager::get_argument_table_from_shader_type(ShaderType shader_type) const
{
    MTL4::ArgumentTable* argument_table;
    switch (shader_type)
    {
        case ShaderType::VertexShader:
            argument_table = vertex_argument_table;
            break;

        case ShaderType::PBRFragment:
            argument_table = pbr_fragment_argument_table;
            break;

        case ShaderType::ToonFragment:
            argument_table = toon_fragment_argument_table;
            break;

        default:
            argument_table = nullptr;
            break;
    };

    return argument_table;
}

void ArgumentTableManager::create_argument_table(ShaderType shader_type)
{
    NS::Error* error = nullptr;
    MTL4::ArgumentTable** argument_table;

    MTL4::ArgumentTableDescriptor* argument_table_descriptor = MTL4::ArgumentTableDescriptor::alloc()->init();

    switch (shader_type)
    {
        case ShaderType::PBRFragment:
        {
            argument_table = &pbr_fragment_argument_table;

            argument_table_descriptor->setMaxBufferBindCount(static_cast<NS::UInteger>(ArgumentSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_BUFFERS));
            argument_table_descriptor->setMaxSamplerStateBindCount(static_cast<NS::UInteger>(ArgumentSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_SAMPLERS));
            argument_table_descriptor->setMaxTextureBindCount(static_cast<NS::UInteger>(ArgumentSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_FRAGMENT_TEXTURES));
            break;
        }

        case ShaderType::ToonFragment:
        {
            argument_table = &toon_fragment_argument_table;

            argument_table_descriptor->setMaxBufferBindCount(static_cast<NS::UInteger>(ArgumentSlots::ToonRenderingArgumentSlot::NUMBER_OF_TOON_FRAGMENT_BUFFERS));
            argument_table_descriptor->setMaxSamplerStateBindCount(static_cast<NS::UInteger>(ArgumentSlots::ToonRenderingArgumentSlot::NUMBER_OF_TOON_FRAGMENT_SAMPLERS));
            argument_table_descriptor->setMaxTextureBindCount(static_cast<NS::UInteger>(ArgumentSlots::ToonRenderingArgumentSlot::NUMBER_OF_TOON_FRAGMENT_TEXTURES));
            break;
        }

        case ShaderType::VertexShader:
        {
            argument_table = &vertex_argument_table;

            argument_table_descriptor->setMaxBufferBindCount(static_cast<NS::UInteger>(ArgumentSlots::PBRRenderingArgumentSlot::NUMBER_OF_PBR_VERTEX_BUFFERS));
            break;
        }

        default:
            argument_table = nullptr;
    }

    // TODO: Add compile time flag checking, and skip this if in prod
    if (argument_table == nullptr)
    {
        printf("Error: No argument table found for shader type %u\n", shader_type);
        assert(nullptr != argument_table);
    }
    else if (*argument_table != nullptr)
    {
        printf("Error: Argument table already exists for shader type %u\n", shader_type);
        assert(nullptr == *argument_table);
    }

    *argument_table = device->newArgumentTable(argument_table_descriptor, &error);

    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    argument_table_descriptor->release();
}