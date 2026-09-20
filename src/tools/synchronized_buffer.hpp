/**
 * @file synchronized_buffer.hpp
 * @brief A thread safe buffer
 */

#pragma once
#include <mutex>
#include <cstdint>

template<typename T>
class SynchronizedBuffer
{
    public:
    SynchronizedBuffer<T>()
    {
        count = 0;
        buffer = nullptr;
    }

    SynchronizedBuffer<T>(SynchronizedBuffer<T>&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::lock_guard<std::mutex> otherlock(other.mutex);
        buffer = other.buffer;
        count = other.count;
        other.count = 0;
        other.buffer = nullptr;
    }

    /**
     * Allocate memory for n items
     */
    SynchronizedBuffer<T>(size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
        count = n;
        buffer = new T[n];
    }

    /**
     * Deallocate buffer memory
     */
    ~SynchronizedBuffer<T>()
    {
        std::lock_guard<std::mutex> lock(mutex);
        delete[] buffer;
    }

    T* lock_mutex_and_move_data()
    {
        std::lock_guard<std::mutex> lock(mutex);
        T* data = buffer;
        buffer = nullptr;
        count = 0;
        return data;
    }

    size_t lock_unlock_mutex_and_get_size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }

    mutable std::mutex mutex;
    T* buffer = nullptr;
    size_t count = 0;
};