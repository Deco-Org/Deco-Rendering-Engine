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

    T* moveData()
    {
        std::lock_guard<std::mutex> lock(mutex);
        T* data = buffer;
        buffer = nullptr;
        count = 0;
        return data;
    }

    void fillData(T* data, size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
        count = std::max(n, count);
        for (size_t i = 0; i < n; ++i)
        {
            buffer[i] = data[i];
        }
    }

    void setSize(size_t n)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (buffer)
        {
            delete[] buffer;
        }
        count = n;
        buffer = new T[n];
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (buffer)
        {
            memset(buffer, 0, count);
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

    mutable std::mutex mutex;
    T* buffer;
    size_t count;
    private:
};