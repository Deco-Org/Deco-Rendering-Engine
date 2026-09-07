/**
 * @file command_allocator_pool.hpp
 * @brief
 */

#pragma once
#include <Metal/Metal.hpp>
#include "core_engine_types.h"

class CommandAllocatorPool
{
public:

    CommandAllocatorPool(MTL::Device* metal_device);
    ~CommandAllocatorPool();

    MTL4::CommandBuffer* get_command_buffer();
    void begin_frame(MTL4::CommandQueue* queue);

    uint64_t get_frame_count() const;

private:

    MTL4::CommandAllocator* allocators[Config::MAX_FRAMES_IN_FLIGHT] = {};
    MTL4::CommandBuffer* command_buffer = nullptr;
    MTL::SharedEvent* frame_event = nullptr;
    uint64_t frame_count = 0;
};