/**
 * @file residency_manager.cpp
 * @brief
 */

#include "residency_manager.hpp"

const NS::String* ResidencyManager::PERSISTENT_SET_LABEL = NS::String::string("Persistent residency set", NS::UTF8StringEncoding);
const NS::String* ResidencyManager::DYNAMIC_SET_LABEL = NS::String::string("Dynamic residency set", NS::UTF8StringEncoding);

ResidencyManager::ResidencyManager(MTL::Device* metal_device, MTL4::CommandQueue* queue)
{
    command_queue = queue;

    // Creating residency sets

    // Persistent set
    MTL::ResidencySetDescriptor* persistent_set_descriptor = MTL::ResidencySetDescriptor::alloc()->init();
    persistent_set_descriptor->setLabel(PERSISTENT_SET_LABEL);
    persistent_set_descriptor->setInitialCapacity(INITIAL_PERSISTENT_SET_CAPACITY);

    NS::Error* error = nullptr;
    persistent_set = metal_device->newResidencySet(persistent_set_descriptor, &error);
    
    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    // Dynamic set
    MTL::ResidencySetDescriptor* dynamic_set_descriptor = MTL::ResidencySetDescriptor::alloc()->init();
    dynamic_set_descriptor->setLabel(DYNAMIC_SET_LABEL);
    dynamic_set_descriptor->setInitialCapacity(INITIAL_DYNAMIC_SET_CAPACITY);

    error = nullptr;
    dynamic_set = metal_device->newResidencySet(dynamic_set_descriptor, &error);
    
    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    command_queue->addResidencySet(persistent_set);
    command_queue->addResidencySet(dynamic_set);

    latest_commit_value = 0;
}

ResidencyManager::~ResidencyManager()
{
    command_queue->removeResidencySet(persistent_set);
    command_queue->removeResidencySet(dynamic_set);
    
    persistent_set->endResidency();
    dynamic_set->endResidency();

    persistent_set->release();
    dynamic_set->release();
}

void ResidencyManager::add_persistent(MTL::Allocation* resource_allocation)
{
    persistent_set_dirty = true;
    persistent_set->addAllocation(resource_allocation);
}

void ResidencyManager::add_dynamic(MTL::Allocation* resource_allocation)
{
    dynamic_set_dirty = true;
    dynamic_set->addAllocation(resource_allocation);
}

void ResidencyManager::remove_dynamic(MTL::Allocation* resource_allocation)
{
    dynamic_set_dirty = true;
    dynamic_set->removeAllocation(resource_allocation);
}

void ResidencyManager::commit()
{
    if (persistent_set_dirty || dynamic_set_dirty)
    {
        if (persistent_set_dirty)
        {
            persistent_set_dirty = false;
            persistent_set->commit();
        }
        else
        {
            dynamic_set_dirty = false;
            dynamic_set->commit();
        }

        latest_commit_value += 1;
    }
}

size_t ResidencyManager::persistent_allocations_count() const
{
    return static_cast<size_t>(persistent_set->allocationCount());
}

size_t ResidencyManager::dynamic_allocations_count() const
{
    return static_cast<size_t>(dynamic_set->allocationCount());
}

bool ResidencyManager::persistent_set_contains_allocation(MTL::Allocation* resource_allocation) const
{
    return persistent_set->containsAllocation(resource_allocation);
}

bool ResidencyManager::dynamic_set_contains_allocation(MTL::Allocation* resource_allocation) const
{
    return dynamic_set->containsAllocation(resource_allocation);
}

bool ResidencyManager::is_persistent_residency_set_dirty() const
{
    return persistent_set_dirty;
}

bool ResidencyManager::is_dynamic_residency_set_dirty() const
{
    return dynamic_set_dirty;
}