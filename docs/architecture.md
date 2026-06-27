# Deco Rendering Engine Architecture

## Layers
The Deco Rendering Engine uses a combination of Object Oriented and Data Oriented Design. Data Oriented Design is used at low levels where efficiency is key (_see [AoS and SoA on Wikipedia](https://en.wikipedia.org/wiki/AoS_and_SoA)_).
### Core Systems
#### Transformation System
This applies transformations
```cpp
using TransformationHandle = uint32_t;

// Used for transforming instances
struct TransformationSystem
{
    std::vector<simd_float3> positions;
    std::vector<simd_quatf> rotations;
    std::vector<simd_float3> scales;
    std::vector<TransformationHandle> parentIndices;
    std::vector<matrix_float4x4> worldMatrices; // computed every frame
    
    MTL::Buffer* transformBuffer = nullptr;

    TransformationHandle add(simd_float3 position, simd_quatf rotation, simd_float3 scale, TransformationHandle parent = NO_PARENT);
    void remove(TransformationHandle handle);
    void update(); // compute worldMatrices from positions / rotations / scale
    void uploadToGPU(); // copy worldMatrices into transformBuffer
};
```

#### Animation System
Uses arrays of structures. There will be very few animated characters in a scene at once, so the ergonomic downsides of using structures as arrays are likely not worth the potential performance gains.
```cpp
using ClipHandle = uint32_t;
using AnimationInstanceHandle = uint32_t;
using SkeletonHandle = uint32_t;
using BoneHandle = uint32_t;

inline constexpr uint32_t NO_PARENT = UINT32_MAX;
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

struct AnimationSystem
{
    std::vector<AnimationClip> clips;
    std::vector<AnimationInstance> animationInstances;
    std::vector<Skeleton> skeletons;
    
    // Holds all the bone matrices of the characters
    MTL::Buffer* boneBuffer = nullptr;

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
};
```
#### Culling System
Used for frustrum culling. Implementation of this is somewhat low priority due to the low number of objects that will be rendered at once, but this can still be useful in case something goes behind the camera. Deco should be as lightweight.
```cpp
struct CullingSystem
{
    std::vector<simd_float3>* boundsMin; // Pointer to mesh system bounds min
    std::vector<simd_float3>* boundsMax; // Pointer to mesh system bounds max
    std::vector<bool> isVisible;
    
    void cull();
};
```

### Asset Systems
#### Mesh System
The mesh system holds the information for all loaded meshes
```cpp
struct MeshSystem
{
    std::vector<MTL::Buffer*> vertexBuffers;
    std::vector<MTL::Buffer*> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of meshes
    std::vector<simd_flaot3> boundsMax; // Max bounds of meshes
    std::vector<bool> isSkinned; // Basically whether or not something has bones
    std::vector<uint32_t> boneCounts;
    
    MeshHandle load(const char* path);
    void unload(MeshHandle handle);
};
```
#### Material System
This holds information about materials.
More research needs to be done on creating Toon and PBR shaders.
```cpp
struct PBRMaterial
{
    MTL::Texture* albedoTexture;
    MTL::Texture* normalTexture;
    MTL::Texture* metallicRoughnessAoTexture;
    simd_float4 baseColorFactor;
    float emission;
};

struct ToonMaterial
{
    MTL::Texture* albedoTexture;
    MTL::Texture* shadowThresholdTexture;
    simd_float4 baseColorFactor;
    float shadowSoftness;
};

struct MaterialSystem
{
    std::vector<PBRMaterial> pbrMaterials;
    std::vector<ToonMaterial> toonMaterials;
    
    MaterialHandle addPBR(const PBRMaterial& material);
    MaterialHandle addToon(const ToonMaterial& material);
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

struct PipelineLibrary
{
    MTL4::RenderPipelineState* pipelineStateObjects[(int)RenderPipeline::Count] = {};
    MTL4::Compiler* compiler = nullptr;
    MTL::Device* device = nullptr;
    
    void init(MTL::Device* metalDevice, MTL4::Compiler* metalCompiler);
    void cleanup();
    
    MTL4::RenderPipelineState* get(RenderPipeline pipeline) const;
    void buildPipelines(MTL::PixelFormat pixelFormat);
};
```
#### Residency Manager
Asset loading should be done on a separate thread from rendering. Residency sets are updated in parallel with encoding, and the command buffer must wait for the residency manager to "commit" before it itself can be committed.
```cpp
struct ResidencyManager
{
    MTL::ResidencySet* persistentSet = nullptr;
    MTL::ResidencySet* dynamicSet = nullptr;
    
    MTL::SharedEvent* commitEvent = nullptr;
    MTL4::CommandQueue* commandQueue = nullptr;
    std::atomic<uint64_t> latestCommitValue = 0;
    
    void init(MTL::Device* device, MTL4::CommandQueue* queue);
    void cleanup();
    
    void addPersistent(MTL::Buffer* buffer);
    void addPersistent(MTL::Texture* texture);
    void addDynamic(MTL::Buffer* buffer);
    void addDynamic(MTL::Texture* texture);
    void removeDynamic(MTL::Buffer* buffer);
    void removeDynamic(MTL::Texture* texture);
    void commit(); // call after adding / removing dynamic resources
    void waitForCommit(uint64_t commitValue);
};
```
#### Command Allocator Pool
Because we can have three frames in flight at once, there should be a pool of three command allocators.
```cpp
// Note that Deco::Config::MAX_FRAMES_IN_FLIGHT is 3
struct CommandAllocatorPool
{
    MTL4::CommandAllocator* allocators[Config::MAX_FRAMES_IN_FLIGHT] = {};
    MTL4::CommandBuffer* commandBuffer = nullptr;
    MTL::SharedEvent* frameEvent = nullptr; // This fires every time a frame finishes
    uint64_t frameCount = 0;
    
    void init(MTL::Device* device);
    void cleanup();
    void beginFrame(MTL4::CommandQueue* queue);
};
```
#### Argument Table Manager
```cpp
struct ArgumentTableManager
{
    MTL4::ArgumentTable* vertexTable = nullptr;
    MTL4::ArgumentTable* fragmentTable = nullptr;
    
    void init(MTL::Device* device);
    void cleanup();
    
    void bindBuffer(MTL::Buffer* buffer, NS::UInteger index);
    void bindTexture(MTL::Texture* texture, NS::UInteger index);
    
    void apply(MTL4::RenderCommandEncoder* encoder);
};
```