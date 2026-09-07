/**
 * @file command_allocator_pool.cpp
 * @brief
 */

#include "command_allocator_pool.hpp"

CommandAllocatorPool::CommandAllocatorPool(MTL::Device* metal_device)
{
    frame_count = 0;
    command_buffer = metal_device->newCommandBuffer();
    frame_event = metal_device->newSharedEvent();
    frame_event->setSignaledValue(frame_count);

    for (uint8_t i = 0; i < Config::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        allocators[i] = metal_device->newCommandAllocator();
    }
}

CommandAllocatorPool::~CommandAllocatorPool()
{
    if (!command_buffer)
        release_command_buffer();
}

bool CommandAllocatorPool::default_waiting_function(CommandAllocatorPool* instance, uint64_t wait_value, uint64_t wait_time_in_milliseconds)
{
    return instance->frame_event->waitUntilSignaledValue(instance->frame_count, FRAME_TIMEOUT_TIME_IN_MILLISECONDS);
}

void CommandAllocatorPool::default_frame_completion_callback_function(CommandAllocatorPool* instance)
{
    uint64_t frame_index = instance->frame_count % Config::MAX_FRAMES_IN_FLIGHT;
    instance->allocators[frame_index]->reset();
    instance->command_buffer->beginCommandBuffer(instance->allocators[frame_index]);

    instance->frame_count += 1;
}

void CommandAllocatorPool::begin_frame()
{
    if (frame_count >= Config::MAX_FRAMES_IN_FLIGHT)
    {
        uint64_t wait_value = frame_count - Config::MAX_FRAMES_IN_FLIGHT;
        default_waiting_function(this, wait_value, FRAME_TIMEOUT_TIME_IN_MILLISECONDS);
    }

    default_frame_completion_callback_function(this);
}

void CommandAllocatorPool::signal_frame_complete(MTL4::CommandQueue* command_queue)
{
    command_queue->signalEvent(frame_event, frame_count);
}

void CommandAllocatorPool::release_command_buffer()
{
    command_buffer->release();
    command_buffer = nullptr;
}

MTL4::CommandBuffer* CommandAllocatorPool::get_command_buffer() const
{
    return command_buffer;
}

uint64_t CommandAllocatorPool::get_frame_count() const
{
    return frame_count;
}