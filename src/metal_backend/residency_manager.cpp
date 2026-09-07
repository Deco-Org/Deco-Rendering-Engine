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
    persistent_set->endResidency();
    dynamic_set->endResidency();
}

void ResidencyManager::add_persistent(MTL::Buffer* buffer)
{
    persistent_set->addAllocation(buffer);
}

void ResidencyManager::add_persistent(MTL::Texture* texture)
{
    persistent_set->addAllocation(texture);
}

void ResidencyManager::add_persistent(MTL::Heap* heap)
{
    persistent_set->addAllocation(heap);
}

void ResidencyManager::add_dynamic(MTL::Buffer* buffer)
{
    dynamic_set->addAllocation(buffer);
}

void ResidencyManager::add_dynamic(MTL::Texture* texture)
{
    dynamic_set->addAllocation(texture);
}

void ResidencyManager::add_dynamic(MTL::Heap* heap)
{
    dynamic_set->addAllocation(heap);
}

void ResidencyManager::remove_dynamic(MTL::Buffer* buffer)
{
    dynamic_set->removeAllocation(buffer);
}

void ResidencyManager::remove_dynamic(MTL::Texture* texture)
{
    dynamic_set->removeAllocation(texture);
}

void ResidencyManager::remove_dynamic(MTL::Heap* heap)
{
    dynamic_set->removeAllocation(heap);
}

void ResidencyManager::commit()
{
    persistent_set->commit();
    dynamic_set->commit();
    latest_commit_value += 1;
}