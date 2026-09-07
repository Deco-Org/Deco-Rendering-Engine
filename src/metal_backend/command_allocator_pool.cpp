/**
 * @file command_allocator_pool.cpp
 * @brief
 */

#include "command_allocator_pool.hpp"

CommandAllocatorPool::CommandAllocatorPool(MTL::Device* metal_device)
{
    command_buffer = metal_device->newCommandBuffer();
}

CommandAllocatorPool::~CommandAllocatorPool()
{
    release_command_buffer();
}

MTL4::CommandBuffer* CommandAllocatorPool::get_command_buffer()
{
    return command_buffer;
}

void CommandAllocatorPool::release_command_buffer()
{
    command_buffer->release();
    command_buffer = nullptr;
}

uint64_t CommandAllocatorPool::get_frame_count() const
{
    return frame_count;
}