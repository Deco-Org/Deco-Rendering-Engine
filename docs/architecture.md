# Deco Rendering Engine Architecture

## Layers
The Deco Rendering Engine uses a combination of Object Oriented and Data Oriented Design. Data Oriented Design is used at low levels where efficiency is key (_see [AoS and SoA on Wikipedia](https://en.wikipedia.org/wiki/AoS_and_SoA)_).
### Core Systems
#### Transformation System
This applies transformations
```cpp
using TransformationHandle = uint32_t;
inline constexpr NO_TRANSFORMATION_PARENT = UINT32_MAX;

// Used for transforming instances
class TransformationSystem
{
    public:
    TransformationHandle add(simd_float3 position, simd_quatf rotation, simd_float3 scale, TransformationHandle parent = NO_TRANSFORMATION_PARENT);
    void remove(TransformationHandle handle);
    void update(); // compute worldMatrices from positions / rotations / scale
    void uploadToGPU(); // copy worldMatrices into transformBuffer
    
    MTL::Buffer* transformBuffer = nullptr;
    
    std::vector<simd_float3> positions;
    std::vector<simd_quatf> rotations;
    std::vector<simd_float3> scales;
    std::vector<TransformationHandle> parentIndices;
    std::vector<matrix_float4x4> worldMatrices; // computed every frame
};
```

#### Animation System
Uses arrays of structures. There will be very few animated characters in a scene at once, so the ergonomic downsides of using structures as arrays are likely not worth the potential performance gains.
```cpp
using ClipHandle = uint32_t;
using AnimationInstanceHandle = uint32_t;
using SkeletonHandle = uint32_t;
using BoneHandle = uint32_t;

inline constexpr BoneHandle NO_BONE_PARENT = UINT32_MAX;
inline constexpr ClipHandle INVALID_CLIP = UINT32_MAX;
inline constexpr AnimationInstanceHandle INVALID_INSTANCE = UINT32_MAX;
inline constexpr SkeletonHandle INVALID_SKELETON = UINT32_MAX;

struct BakedKeyframe
{
    float time;
    simd_float3 translation;
    simd_quatf rotation;
    simd_float3 scale;
};

struct BakedBoneTrack
{
    std::vector<BakedKeyframe> keyframes;
};

/**
 * Shared across instances
*/
struct AnimationClip
{
    std::string name;
    float duration;
    SkeletonHandle skeletonHandle;
    std::vector<BakedBoneTrack> boneTracks;
};

struct AnimationInstance
{
    ClipHandle clipIndex;
    float currentTime;
    bool isPlaying;
    bool loops;
    float playbackSpeed;
    std::vector<matrix_float4x4> boneMatrices;
    std::vector<matrix_float4x4> skinningMatrices;
};


struct Skeleton
{
    std::vector<matrix_float4x4> inverseBindMatrices;
    std::vector<BoneHandle> parentIndices;
    uint32_t boneCount;
};

class AnimationSystem
{
    public:

    AnimationSystem(MTL::Device *device);
    
    ClipHandle addClip(ufbx_scene* scene, ufbx_anim_stack* animation);
    AnimationInstanceHandle addInstance(ClipHandle clipIndex);
    SkeletonHandle addSkeleton(ufbx_scene* scene, ufbx_skin_deformer* skin);
    void play(AnimationInstanceHandle instance);
    void play(AnimationInstanceHandle instance, ClipHandle clip, bool loop = false);
    void stop(AnimationInstanceHandle instance);
    void update(float deltaTime); // advance all playing instances
    void setPlaybackSpeed(AnimationInstanceHandle instance, float speed);
    float getPlaybackTime(AnimationInstanceHandle instance);
    void uploadToGPU();
    
    // Holds all the bone matrices of the characters
    MTL::Buffer* boneBuffer = nullptr;

    std::vector<AnimationClip> clips;
    std::vector<AnimationInstance> animationInstances;
    std::vector<Skeleton> skeletons;
};
```
#### Culling System
Used for frustrum culling. Implementation of this is somewhat low priority due to the low number of objects that will be rendered at once, but this can still be useful in case something goes out of view.
```cpp
class CullingSystem
{
    public:
    CullingSystem(std::vector<simd_float3>* min, std::vector<simd_float3>* max);

    void cull();
    void addBoundsArray(std::vector<simd_float3>* min, std::vector<simd_float3>* max);
    
    std::vector<simd_float3>* boundsMin = nullptr; // Pointer to mesh system bounds min
    std::vector<simd_float3>* boundsMax = nullptr; // Pointer to mesh system bounds max
    std::vector<bool> isVisible;
};
```

### Asset Systems
#### Mesh System
The mesh system holds the information for all loaded meshes
```cpp
using MeshHandle = uint32_t;

class MeshSystem
{
    public:
    MeshHandle load(const char* path);
    void unload(MeshHandle handle);
    
    std::vector<MTL::Buffer*> vertexBuffers;
    std::vector<MTL::Buffer*> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of meshes
    std::vector<simd_float3> boundsMax; // Max bounds of meshes
    std::vector<bool> isSkinned; // Basically whether or not something has bones
    std::vector<uint32_t> boneCounts;
};
```
#### Material System
This holds information about materials.
More research needs to be done on creating Toon and PBR shaders.
```cpp
using MaterialHandle = uint32_t;

struct PBRMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* normalTexture = nullptr;
    MTL::Texture* metallicRoughnessAoTexture = nullptr;
    MTL::Texture* emission = nullptr;
    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct ToonMaterial
{
    MTL::Texture* albedoTexture = nullptr;
    MTL::Texture* shadowThresholdTexture = nullptr;
    simd_float4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float shadowSoftness = 0.0f;
};

class MaterialSystem
{
    public:
    PBRMaterial getPBR(MaterialHandle handle) const;
    ToonMaterial getToon(MaterialHandle handle) const;
    MaterialHandle addPBR(const PBRMaterial& material);
    MaterialHandle addToon(const ToonMaterial& material);
    
    std::vector<PBRMaterial> pbrMaterials;
    std::vector<ToonMaterial> toonMaterials;
};
```
### Metal Backend
Potential future optimizations include multithreading the encoding of command buffers.
#### Pipeline Manager
```cpp
enum class RenderPipeline
{
    PBRStatic = 0,
    PBRSkinned = 1,
    ToonStatic = 2,
    ToonSkinned = 3,
    Count
};

class PipelineLibrary
{
    public:
    PipelineLibrary(MTL::Device* metalDevice, MTL4::Compiler* metalCompiler);
    ~PipelineLibrary();

    MTL4::RenderPipelineState* get(RenderPipeline pipeline) const;
    void buildPipelines(MTL::PixelFormat pixelFormat);
    
    private:
    MTL4::RenderPipelineState* pipelineStateObjects[(int)RenderPipeline::Count] = {};
    MTL4::Compiler* compiler = nullptr;
    MTL::Device* device = nullptr;
};
```
#### Residency Manager
Asset loading should be done on a separate thread from rendering. Residency sets are updated in parallel with encoding, and the command buffer must wait for the residency manager to "commit" before it itself can be committed.
```cpp
class ResidencyManager
{
    public:
    ResidencyManager(MTL::Device* device, MTL4::CommandQueue* queue);
    ~ResidencyManager();
    
    void addPersistent(MTL::Buffer* buffer);
    void addPersistent(MTL::Texture* texture);
    void addDynamic(MTL::Buffer* buffer);
    void addDynamic(MTL::Texture* texture);
    void removeDynamic(MTL::Buffer* buffer);
    void removeDynamic(MTL::Texture* texture);
    void commit(); // call after adding / removing dynamic resources
    void waitForCommit(uint64_t commitValue);
    
    private:
    MTL::ResidencySet* persistentSet = nullptr;
    MTL::ResidencySet* dynamicSet = nullptr;
    
    MTL::SharedEvent* commitEvent = nullptr;
    MTL4::CommandQueue* commandQueue = nullptr;
    std::atomic<uint64_t> latestCommitValue = 0;
};
```
#### Command Allocator Pool
Because we can have three frames in flight at once, there should be a pool of three command allocators.
```cpp
// Note that Deco::Config::MAX_FRAMES_IN_FLIGHT is 3
class CommandAllocatorPool
{
    public:
    CommandAllocatorPool(MTL::Device* device);
    ~CommandAllocatorPool();

    MTL4::CommandBuffer* getCommandBuffer();
    uint64_t getFrameCount();
    void beginFrame(MTL4::CommandQueue* queue);
    
    private:
    MTL4::CommandAllocator* allocators[Config::MAX_FRAMES_IN_FLIGHT] = {};
    MTL4::CommandBuffer* commandBuffer = nullptr;
    MTL::SharedEvent* frameEvent = nullptr; // This fires every time a frame finishes
    uint64_t frameCount = 0;
};
```
#### Argument Table Manager
```cpp
class ArgumentTableManager
{
    public:
    ArgumentTableManager(MTL::Device* device);
    ~ArgumentTableManager();
    
    void bindBuffer(MTL::Buffer* buffer, NS::UInteger index);
    void bindTexture(MTL::Texture* texture, NS::UInteger index);
    void applyTables(MTL4::RenderCommandEncoder* encoder);
    
    private:
    MTL4::ArgumentTable* vertexTable = nullptr;
    MTL4::ArgumentTable* fragmentTable = nullptr;
};
```