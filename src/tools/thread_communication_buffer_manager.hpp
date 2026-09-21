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

    template <typename T, typename H, typename E>
    class BufferManager
    {
    public:

        struct EntryWithHandle
        {
            T data;
            H handle;
        };

        // template <HasItemAndHandleFields<T, H> E>
        void add_to_additions_buffer(E* entries, size_t count, H max_handle)
        {
            add_to_buffer(
                additions_input,
                entries,
                count,
                max_handle
            );
        }

        SystemInputBuffer<E, H> additions_input;
        SystemOutputBuffer<H> additions_output;
        SynchronizedBuffer<H> removals_input;
    };
};