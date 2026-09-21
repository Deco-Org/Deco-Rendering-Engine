/**
 * @file render_thread_queue_manager.hpp
 * @brief
 */

#include "core_engine_types.h"
#include "synchronized_buffer.hpp"

template <typename E, typename Elem, typename Handle>
concept HasItemAndHandleFields = requires(const E& e)
{
    { e.item } -> std::convertible_to<Elem>;
    { e.handle } -> std::convertible_to<Handle>;
};

namespace ThreadCommunication
{

    /**
     * @note Involves locking and unlocking the buffer's mutex
     */
    template <typename T, typename B>
        requires DecoThreadSafeBuffer<B, T>
    void add_to_buffer(B& buffer, const T* data, size_t count)
    {
        std::lock_guard lock(buffer.mutex);
        
        // Critical section
        size_t old_size = buffer.count;
        if (old_size > 0)
        {
            const size_t new_size = old_size + count;
            T* temp = buffer.buffer;
            buffer.buffer = new T[new_size];
            memcpy(buffer.buffer, temp, old_size * sizeof(T));
            memcpy(buffer.buffer + old_size, data, count * sizeof(T));
            buffer.count = new_size;
            delete[] temp;
        }
        else
        {
            if (buffer.buffer)
                delete[] buffer.buffer;
            buffer.buffer = new T[count];
            memcpy(buffer.buffer, data, count * sizeof(T));
            buffer.count = count;
        }
    }

    /**
     * @note Involves locking and unlocking the buffer's mutex
     * @param buffer The buffer that the item is to be added to
     * @param data The items to be added to the buffer
     * @param count The number of items to be added to the buffer
     * @param max_handle The value to be written to the buffer's `max_handle` field.
     */
    template <typename T, typename B, typename H>
        requires DecoThreadSafeBuffer<B, T>
    void add_to_buffer(B& buffer, const T* data, size_t count, H max_handle)
    {
        std::lock_guard lock(buffer.mutex);
        
        // Critical section
        size_t old_size = buffer.count;
        buffer.max_handle = max_handle;

        if (old_size > 0)
        {
            const size_t new_size = old_size + count;
            T* temp = buffer.buffer;
            buffer.buffer = new T[new_size];
            memcpy(buffer.buffer, temp, old_size * sizeof(T));
            memcpy(buffer.buffer + old_size, data, count * sizeof(T));
            buffer.count = new_size;
            delete[] temp;
        }
        else
        {
            if (buffer.buffer)
                delete[] buffer.buffer;
            buffer.buffer = new T[count];
            memcpy(buffer.buffer, data, count * sizeof(T));
            buffer.count = count;
        }
    }

    template <typename T, typename H, typename B>
        requires DecoThreadSafeBuffer<B, T>
    H drain_buffer_and_get_max_handle(B& buffer, T** entries_output, size_t* count)
    {
        if (entries_output == nullptr)
        {
            std::lock_guard lock(buffer.mutex);
                
            // Critical section
            if (count)
                *count = buffer.count;

            delete[] buffer.buffer;
            buffer.buffer = nullptr;
            buffer.count = 0;
            
            return buffer.max_handle;
        }
        else
        {
            std::lock_guard lock(buffer.mutex);

            // Critical section
            if (count)
                *count = buffer.count;
            
            *entries_output = buffer.buffer;
            buffer.buffer = nullptr;
            buffer.count = 0;
            return buffer.max_handle;
        }
    }

    template <typename T, typename Handle, typename E>
    class BufferManager
    {
    public:

        void add_to_additions_input_buffer(E* entries, size_t count, Handle max_handle)
        {
            add_to_buffer(
                additions_input,
                entries,
                count,
                max_handle
            );
        }

        void add_to_removals_buffer(Handle* handles, size_t count)
        {
            add_to_buffer(
                removals_input,
                handles,
                count
            );
        }

        /**
         * @warning This should only be called on the render thread.
         */
        void add_to_additions_output_buffer(Handle* handles, size_t count, size_t max_handle)
        {
            add_to_buffer(
                additions_output,
                handles,
                count,
                max_handle
            );
        }

        /**
         * @warning This should only be called on the render thread.
         */
        Handle drain_additions_input_buffer(E** entries_output, size_t* count)
        {
            std::lock_guard lock(additions_input.mutex);
            
            // Critical section
            if (entries_output == nullptr)
            {
                delete[] additions_input.buffer;
                if (count)
                    *count = additions_input.count;
            }
            else
            {
                *entries_output = additions_input.buffer;
                *count = additions_input.count;
            }

            additions_input.buffer = nullptr;
            additions_input.count = 0;
            return additions_input.max_handle;
        }

        Handle drain_additions_output_buffer(Handle** handles_output, size_t* count)
        {
            std::lock_guard lock(additions_output.mutex);
            
            // Critical section
            if (handles_output == nullptr)
            {
                delete[] additions_output.buffer;
                if (count)
                    *count = additions_output.count;
            }
            else
            {
                *handles_output = additions_output.buffer;
                *count = additions_output.count;
            }

            additions_output.buffer = nullptr;
            additions_output.count = 0;
            return additions_output.max_handle;
        }

        SystemInputBuffer<E, Handle> additions_input;
        SystemOutputBuffer<Handle> additions_output;
        SynchronizedBuffer<Handle> removals_input;
    };
};