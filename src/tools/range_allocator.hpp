/**
 * @file range_allocator.hpp
 * @brief 
 */

#pragma once
#include <vector>
#include <optional>

struct AllocationRange
{
    uint32_t start;
    uint32_t size;
};

class RangeAllocator
{
public:
    explicit RangeAllocator(const uint32_t capacity);

    std::optional<AllocationRange> alloc(const uint32_t size, const uint32_t alignment = 1);
    void free(AllocationRange range);
    uint32_t capacity() const;

private:
    struct FreeBlock
    {
        uint32_t start;
        uint32_t size;

        bool operator<(const FreeBlock& other) const
        {
            return start < other.start;
        }
    };

    uint32_t total_capacity;
    std::vector<FreeBlock> free_blocks;
};