/**
 * @file residency_manager.cpp
 * @brief
 */

#include "residency_manager.hpp"

const NS::String* ResidencyManager::persistent_set_label = NS::String::string("Persistent residency set", NS::UTF8StringEncoding);
const NS::String* ResidencyManager::dynamic_set_label = NS::String::string("Dynamic residency set", NS::UTF8StringEncoding);

ResidencyManager::ResidencyManager(MTL::Device* metal_device, MTL4::CommandQueue* queue)
{
    command_queue = queue;

    // Creating residency sets

    // Persistent set
    MTL::ResidencySetDescriptor* persistent_set_descriptor = MTL::ResidencySetDescriptor::alloc()->init();
    persistent_set_descriptor->setLabel(persistent_set_label);
    persistent_set_descriptor->setInitialCapacity(16);

    NS::Error* error = nullptr;
    persistent_set = metal_device->newResidencySet(persistent_set_descriptor, &error);
    
    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    MTL::ResidencySetDescriptor* dynamic_set_descriptor = MTL::ResidencySetDescriptor::alloc()->init();
    dynamic_set_descriptor->setLabel(dynamic_set_label);
    dynamic_set_descriptor->setInitialCapacity(512);

    error = nullptr;
    dynamic_set = metal_device->newResidencySet(dynamic_set_descriptor, &error);
    
    if (error)
    {
        printf("Error: %s\n", error->localizedDescription()->cString(NS::UTF8StringEncoding));
        assert(nullptr == error);
    }

    command_queue->addResidencySet(dynamic_set);
    command_queue->addResidencySet(persistent_set);

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
    persistent_set->addAllocation(resource_allocation);
}

void ResidencyManager::add_dynamic(MTL::Allocation* resource_allocation)
{
    dynamic_set->addAllocation(resource_allocation);
}

void ResidencyManager::remove_dynamic(MTL::Allocation* resource_allocation)
{
    dynamic_set->removeAllocation(resource_allocation);
}

void ResidencyManager::commit()
{
    persistent_set->commit();
    dynamic_set->commit();
    latest_commit_value += 1;
}

size_t ResidencyManager::persistent_allocations_count() const
{
    return static_cast<size_t>(persistent_set->allocationCount());
}

size_t ResidencyManager::dynamic_allocations_count() const
{
    return static_cast<size_t>(dynamic_set->allocationCount());
}