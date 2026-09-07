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

    void add_persistent(MTL::Buffer* buffer);
    void add_persistent(MTL::Texture* texture);
    void add_persistent(MTL::Heap* heap);

    void add_dynamic(MTL::Buffer* buffer);
    void add_dynamic(MTL::Texture* texture);
    void add_dynamic(MTL::Heap* heap);

    void remove_dynamic(MTL::Buffer* buffer);
    void remove_dynamic(MTL::Texture* texture);
    void remove_dynamic(MTL::Heap* heap);

    void commit();

    std::atomic<uint64_t> latest_commit_value = 0;

    static const NS::String* persistent_set_label;
    static const NS::String* dynamic_set_label;

private:
    MTL::ResidencySet* persistent_set = nullptr;
    MTL::ResidencySet* dynamic_set = nullptr;
    MTL4::CommandQueue* command_queue = nullptr;
};