/**
 * @file residency_manager.hpp
 * @brief
 */

#include <Metal/Metal.hpp>
#include <atomic>

class ResidencyManager
{
public:
    ResidencyManager(MTL::Device* metal_device, MTL4::CommandQueue* queue);
    ~ResidencyManager();

    /**
     * Adds a resource to the persistent residency set.
     * 
     * The persistent residency set is a set of resource allocations that are persistent across the lifetime of the program. The persistent residency set should be populated on startup. Items added to the persistent residency set cannot be removed.
     * 
     * @param resource_allocation
     */
    void add_persistent(MTL::Allocation* resource_allocation);

    /**
     * Adds a resource to the dynamic residency set.
     * 
     * @param resource_allocation
     */
    void add_dynamic(MTL::Allocation* resource_allocation);

    /**
     * Removes a resource from the dynamic residency set
     * 
     * @param resource_allocation
     */
    void remove_dynamic(MTL::Allocation* resource_allocation);

    void commit();

    size_t persistent_allocations_count() const;
    size_t dynamic_allocations_count() const;

    bool dyanmic_set_contains_allocation(MTL::Allocation* resource_allocation) const;
    bool persistent_set_contains_allocation(MTL::Allocation* resource_allocation) const;

    std::atomic<uint64_t> latest_commit_value = 0;

    static const NS::String* persistent_set_label;
    static const NS::String* dynamic_set_label;

private:
    MTL::ResidencySet* persistent_set = nullptr;
    MTL::ResidencySet* dynamic_set = nullptr;
    MTL4::CommandQueue* command_queue = nullptr;
};