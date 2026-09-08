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

    static bool default_waiting_function(CommandAllocatorPool* instance, uint64_t wait_value, uint64_t wait_time_in_milliseconds);
    static void default_frame_completion_callback_function(CommandAllocatorPool* instance);

    void begin_frame();

    template<std::invocable<CommandAllocatorPool*, uint64_t, uint64_t> WaitFn, std::invocable<CommandAllocatorPool*> CbFn>
    void begin_frame(WaitFn waiting_function, CbFn callback)
    {
        if (frame_count >= Config::MAX_FRAMES_IN_FLIGHT)
            waiting_function(this, frame_count - Config::MAX_FRAMES_IN_FLIGHT, FRAME_TIMEOUT_TIME_IN_MILLISECONDS);
        callback(this);
    }

    /**
     * Enqueue a signal in the command queue
     */
    void signal_frame_complete(MTL4::CommandQueue* command_queue);
    
    /**
     * Releases the command buffer.
     * This method is automatically called on deconstruction.
     */
    void release_command_buffer();
    
    MTL4::CommandBuffer* get_command_buffer() const;
    uint64_t get_frame_count() const;

    static constexpr size_t FRAME_TIMEOUT_TIME_IN_MILLISECONDS = 1000UZ;

private:

    MTL4::CommandAllocator* allocators[Config::MAX_FRAMES_IN_FLIGHT] = {};
    MTL4::CommandBuffer* command_buffer = nullptr;
    MTL4::CommandQueue* command_queue = nullptr;
    MTL::SharedEvent* frame_event = nullptr;
    uint64_t frame_count = 0;
};