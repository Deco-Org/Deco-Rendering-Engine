/**
 * @file range_allocator.cpp
 * @brief 
 */

#include "range_allocator.hpp"

RangeAllocator::RangeAllocator(const uint32_t capacity) : total_capacity(capacity)
{
    free_blocks.push_back({ 0, capacity });
}

std::optional<AllocationRange> RangeAllocator::alloc(const uint32_t size, const uint32_t alignment = 1)
{
    for (auto it = free_blocks.begin(); it != free_blocks.end(); ++it)
    {
        uint32_t aligned_start = ((it->start + alignment - 1) / alignment) * alignment;
        uint32_t padding = aligned_start - it->start;

        if (it->size < padding + size) continue;

        AllocationRange range = { aligned_start, size };

        if (!padding)
        {
            if (it->size == size)
            {
                free_blocks.erase(it);
            }
            else
            {
                it->start += size;
                it->size -= size;
            }
        }
        else
        {
            uint32_t block_size = it->size;
            it->size = padding;

            uint32_t leftover = block_size - (padding + size);
            if (leftover > 0)
            {
                free_blocks.insert(it + 1, { aligned_start + size, leftover });
            }
        }

        return range;
    }

    return std::nullopt;
}

void RangeAllocator::free(AllocationRange range)
{
    free_blocks.push_back({ range.start, range.size });
    std::sort(free_blocks.begin(), free_blocks.end());

    std::vector<FreeBlock> merged_blocks;

    for (const auto& block : free_blocks)
    {
        if (merged_blocks.empty())
        {
            merged_blocks.push_back(block);
        }
        else
        {
            auto& last_block = merged_blocks.back();
            if (last_block.start + last_block.size == block.start)
            {
                last_block.size += block.size;
            }
            else
            {
                // if last start + last size > block start, we overlapped blocks (alloc is bad)
                // not asserting because I want to make logger first
                merged_blocks.push_back(block);
            }
        }
    }

    free_blocks = std::move(merged_blocks);
}

uint32_t RangeAllocator::capacity() const
{
    return total_capacity;
}