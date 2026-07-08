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
    /**
     * Allocate memory for n items
     */
    SynchronizedBuffer<T>(size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
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

    void set(size_t index, T item)
    {
        std::lock_guard<std::mutex> lock(mutex);
        buffer[index] = item;
    }

    T at(size_t index) const 
    {
        std::lock_guard<std::mutex> lock(mutex);
        return buffer[index];
    }

    // T& operator[](size_t index);
    // const T& operator[](size_t index) const;
    
    private:
    mutable std::mutex mutex;
    T* buffer;
    size_t size;
};