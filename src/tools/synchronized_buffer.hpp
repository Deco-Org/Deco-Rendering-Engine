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
        buffer = nullptr;
    }

    SynchronizedBuffer<T>(SynchronizedBuffer<T>&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::lock_guard<std::mutex> otherlock(other.mutex);
        buffer = other.buffer;
        numberOfItems = other.numberOfItems;
        other.numberOfItems = 0;
        other.buffer = nullptr;
    }

    /**
     * Allocate memory for n items
     */
    SynchronizedBuffer<T>(size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
        numberOfItems = n;
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

    T* moveData()
    {
        std::lock_guard<std::mutex> lock(mutex);
        T* data = buffer;
        buffer = nullptr;
        numberOfItems = 0;
        return data;
    }

    void setSize(size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (buffer)
        {
            delete[] buffer;
        }
        numberOfItems = n;
        buffer = new T[n];
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return numberOfItems;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (buffer)
        {
            memset(buffer, 0, numberOfItems);
        }
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
    size_t numberOfItems;
};