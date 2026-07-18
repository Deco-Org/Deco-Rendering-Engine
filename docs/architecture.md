# Deco Rendering Engine Architecture
## Table of Contents
- [Layers](#Layers)
    - [Core Systems](#core-systems)
        - [Transformation System](#transformation-system)
        - [Animation System](#animation-system)
        - [Culling System](#culling-system)
    - [Asset Systems](#asset-systems)
        - [Submesh System](#submesh-system)
        - [Material System](#material-system)
    - [Scene Systems](#scene-systems)
        - [Scene Object System](#scene-object-system)
        - [Camera](#camera)
    - [Metal Backend](#metal-backend)
        - [Pipeline Manager](#pipeline-manager)
        - [Residency Manager](#residency-manager)
        - [Command Allocator Pool](#command-allocator-pool)
        - [Argument Table Manager](#argument-table-manager)
- [Drawing](#drawing)
    - [Drawing Structs and Classes](#drawing-structs-and-classes)
        - [Draw Submesh Command Descriptor](#draw-submesh-command-descriptor)
        - [Render Queue](#render-queue)
            - [Sorting the Render Queue](#sorting-the-render-queue)
    - [Draw Function](#draw-function)
- [Loading Assets](#loading-assets)
- [Unloading Assets](#unloading-assets)
    - [Deletion Queue](#deletion-queue)

## Layers
The Deco Rendering Engine uses a combination of Object Oriented and Data Oriented Design. Data Oriented Design is used at low levels where efficiency is key (_see [AoS and SoA on Wikipedia](https://en.wikipedia.org/wiki/AoS_and_SoA)_).
### Core Systems
#### Transformation System
This applies transformations.
Because child transformations are affected by their parents, parent transformations must be calculated before their children.
```cpp
using TransformationHandle = uint32_t;
inline constexpr TransformationHandle NO_TRANSFORMATION_PARENT = UINT32_MAX;

// Used for transforming instances
class TransformationSystem
{
    public:
    /**
     * Add a transformation to the Transformation System
     */
    TransformationHandle add(Transformation transformation, TransformationHandle parent = NO_TRANSFORMATION_PARENT);
    
    /**
     * Remove a transformation from the Transformation System
     */
    void remove(TransformationHandle handle);
    void computeWorldMatrices(); // compute worldMatrices from positions / rotations / scale
    void updateWorldMatrixBuffer(); // copy worldMatrices into transformationBuffer
    void setParent(TransformationHandle transformation, TransformationHandle parent);
    
    MTL::Buffer* transformationBuffer = nullptr;
    
    std::vector<simd_float3> positions;
    std::vector<simd_quatf> rotations;
    std::vector<simd_float3> scales;
    
    std::vector<TransformationHandle> parentHandles;
    std::vector<matrix_float4x4> worldMatrices; // computed every frame
    
    std::vector<uint32_t> handleToIndex; // Maps handles to the indices in the arrays
    std::vector<TransformationHandle> indexToHandle; // Maps indices in the arrays to handles

    SynchronizedBuffer<TransformationEntry> renderThreadInputBuffer;
    SynchronizedBuffer<TrasnformationHandle> renderThreadOutputBuffer;
};
```

```cpp
struct TransformationEntry
{
    Transformation transformation = {};
    TransformationHandle parent = NO_TRANSFORMATION_PARENT;
};
```

```cpp
// Within CoreEngineTypes.h
struct Transformation
{
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
};
```

Removing and reparenting items are not trivial, as we want to keep our arrays dense and we need to ensure that parent transformations are calculated before child transformations. To do this:
- Handles are mapped to indices in the arrays
- When a transformation is added, it is added to the end of the array.
- When a transformation is removed, everything after the removed transformation must be shifted. At worst, this results in $O(n)$ time complexity.
- When a transformation receives a new parent:
    - If the transformation is before the new parent, everything from the transformation until the new parent must be moved to be after the new parent. If implemented using `memmove()` and a large temporary block of data, this could have $O(k)$ space complexity at the worst case, where $k$ is the amount of memory between the transformation and the new parent.
    - If the transformation is after the new parent, everything's good! No shifting needed
    - Otherwise:
  		- Mark the child's index in a list of "wall indices"
		- Get all the ancestors of the parent node
		- For each ancestor:
			- If the ancestor's index is greater than the child's index but less than the parent's index:
				- Store it in a temporary array
				- Mark the index in a list of "wall indices"
		- Mark the index before the child as the "final wall" (this isn't ever actually used, so it's ok if it's -1)
		- Start at the back
		- For each wall index:
			- Shift everything between the two wall indices over by the number of wall indices encountered. For example, everything betwen the child node and the rightmost ancestor should be shifted to the right by 1, and everything between the rightmost ancestor and the second rightmost ancestor should be shifted over by two.
			- Put the array of the child's ancestors (including the new parent) at the newly opened slots just before the child.

Issues with this solution include:
- Indirection: Instead of simply looking up an item in the array using the handle as the index, getting a transformation instead requires getting the index associated with the handle, then looking up the transformation by index.

#### Animation System
Uses arrays of structures. There will be very few animated characters in a scene at once, so the ergonomic downsides of using structures as arrays are likely not worth the potential performance gains.
```cpp
using ClipHandle = uint32_t;
using AnimationInstanceHandle = uint32_t;
using SkeletonHandle = uint32_t;

inline constexpr uint32_t NO_BONE_PARENT = UINT32_MAX;
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
    ClipHandle clipHandle;
    SkeletonHandle skeletonHandle
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
    std::vector<uint32_t> parentIndices;
    uint32_t boneCount;
};

class AnimationSystem
{
    public:
    
    AnimationSystem(MTL::Device *device);
    
    float getPlaybackTime(AnimationInstanceHandle instance) const;
    Transformation sampleTrack(const BakedBoneTrack& track, float time) const;
    
    ClipHandle addClip(ufbx_scene* scene, ufbx_anim_stack* animation);
    AnimationInstanceHandle addInstance(ClipHandle clipIndex);
    SkeletonHandle addSkeleton(ufbx_scene* scene, ufbx_skin_deformer* skin);
    void removeClip(ClipHandle clip);
    void removeInstance(AnimationInstanceHandle animationInstance);
    void removeSkeleton(SkeletonHandle skeleton);
    
    void play(AnimationInstanceHandle instance);
    void play(AnimationInstanceHandle instance, ClipHandle clip, bool loop = false);
    void stop(AnimationInstanceHandle instance);
    void update(float deltaTime); // advance all playing instances
    void setPlaybackSpeed(AnimationInstanceHandle instance, float speed);
    void updateSkinningBuffer();
    
    // Holds all the skinning matrices of the characters
    MTL::Buffer* skinningBuffer = nullptr;
    
    std::vector<AnimationClip> clips;
    std::vector<AnimationInstance> animationInstances;
    std::vector<Skeleton> skeletons;
    
    private:
    std::vector<ClipHandle> freeClipHandles;
    std::vector<AnimationInstanceHandle> freeAnimationInstanceHandles;
    std::vector<SkeletonHandle> freeSkeletonHandles;
};
```

#### Culling System
Used for frustrum culling.
```cpp
class CullingSystem
{
    public:
    CullingSystem(std::vector<simd_float3>* min, std::vector<simd_float3>* max);
    
    void cull();
    
    std::vector<simd_float3>* boundsMin = nullptr; // Pointer to submesh system bounds min
    std::vector<simd_float3>* boundsMax = nullptr; // Pointer to submesh system bounds max
    std::vector<bool> isVisible;
};
```

### Asset Systems
#### Submesh System
The submesh system holds the information for all loaded submeshes.
```cpp
using SubmeshHandle = uint32_t;
using MeshHandle = uint32_t;

class SubmeshSystem
{
    public:
    SubmeshHandle load(ufbx_mesh_part* part);
    void unload(SubmeshHandle handle);
    
    std::vector<MTL::Buffer*> vertexBuffers;
    std::vector<MTL::Buffer*> indexBuffers;
    std::vector<NS::UInteger> indexCounts;
    std::vector<simd_float3> boundsMin; // Min bounds of submeshes
    std::vector<simd_float3> boundsMax; // Max bounds of submeshes
    std::vector<bool> isSkinned; // Basically whether or not something has bones
    std::vector<uint32_t> boneCounts;

    private:
    std::vector<SubmeshHandle> freeSubmeshHandles;
};
```
There is also a Mesh System, which pretty much only exists for user convenience.
```cpp
struct MeshInfo
{
    SubmeshHandle* submeshHandles;
    MaterialHandle* materialHandles;
    MaterialType* materialTypes;
    size_t submeshCount;
};

class MeshSystem
{
    public:
    MeshHandle add(std::vector<SubmeshHandle> submeshes);
    MeshHandle add(MeshInfo mesh);
    void remove(MeshHandle meshHandle);

    MeshInfo getMeshInfo(MeshHandle meshHandle);

    private:
    std::vector<MeshInfo> meshes;
}
```

#### Material System
This holds information about materials.
More research needs to be done on creating Toon and PBR shaders.

```cpp
using MaterialHandle = uint16_t;
inline constexpr MaterialHandle INVALID_MATERIAL = UINT16_MAX;

enum class MaterialType
{
    PBR,
    Toon,
};

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

struct Material
{
    MaterialType type;
    union
    {
        PBRMaterial pbrMaterial;
        ToonMaterial toonMaterial;
    };
};

class MaterialSystem
{
    public:
    PBRMaterial getPBR(MaterialHandle handle) const;
    ToonMaterial getToon(MaterialHandle handle) const;
    MaterialHandle addPBR(const PBRMaterial& material, bool isOpaque = true);
    MaterialHandle addToon(const ToonMaterial& material, bool isOpaque = true);

    void updateMaterial(MaterialHandle handle, Material material);
    
    std::vector<PBRMaterial> pbrMaterials;
    std::vector<ToonMaterial> toonMaterials;
    
    private:
    std::vector<MaterialHandle> freePBRMaterialHandles;
    std::vector<MaterialHandle> freeToonMaterialHandles;
};
```

### Scene Systems

#### Scene Object System
```cpp
using SceneObjectHandle = uint32_t;

struct SceneObject
{
    SubmeshHandle submeshHandle;
    TransformationHandle transformationHandle;
    AnimationInstanceHandle animationInstanceHandle;
    MaterialHandle materialHandle;
};

class SceneObjectSystem
{
    public:
    std::vector<SubmeshHandle> submeshHandles;
    std::vector<TransformationHandle> transformationHandles;
    std::vector<AnimationInstanceHandle> animationInstanceHandles;
    std::vector<MaterialHandle> materialHandles;
    std::vector<RenderPipeline> pipelineFlags;
    std::vector<uint64_t> staticSortKeyParts; // Sort keys are built off of these based on per-frame calculations
    uint32_t numberOfSceneObjects() const { return submeshHandles.size(); }
    
    SceneObjectHandle add(SceneObject sceneObject);
    void remove(SceneObjectHandle handle);
};
```

#### Camera
```cpp
class Camera
{
    public:
    Camera(
        TransformationHandle handle, 
        float w = 600,
        float h = 400,
        float fov = 90 * (M_PI / 180), 
        float nZ = 0.1f, 
        float fZ = 100.0f);
    
    simd_float4x4 getViewMatrix(const TransformationSystem& transformationSystem) const;
    simd_float4x4 getPerspectiveMatrix() const;
    TransformationHandle getTransformationHandle() const;
    
    void setFov(float fov); // fov is in degrees
    void setNearZ(float nz);
    void setFarZ(float fz);
    void setWidth(float width);
    void setHeight(float height);
    void setTransformationHandle(TransformationHandle handle);

    private:
    TransformationHandle transformationHandle;
    float width;
    float height;
    float aspectRatio() const { return width / height; }
    float fieldOfView; // This is stored in radians
    float nearZ;
    float farZ;
};
```

### Metal Backend
Potential future optimizations include multithreading the encoding of command buffers.
#### Pipeline Manager
```cpp
using RenderPipeline = uint8_t;
namespace RenderPipelineFlags {
    inline constexpr uint8_t Toon = 1 << 0;
    inline constexpr uint8_t Skinned = 1 << 1;
    inline constexpr uint8_t Translucent = 1 << 2;

    inline constexpr uint8_t FlagCount = 3;
    inline constexpr uint8_t PipelineCount = 1 << FlagCount;
}

class PipelineLibrary
{
    public:
    PipelineLibrary(MTL::Device* metalDevice, MTL4::Compiler* metalCompiler);
    ~PipelineLibrary();

    MTL4::RenderPipelineState* get(RenderPipeline pipeline) const;
    void buildPipelines(MTL::PixelFormat pixelFormat);
    
    private:
    MTL4::RenderPipelineState* pipelineStateObjects[(int)RenderPipelineFlags::PipelineCount] = {};
    MTL4::Compiler* compiler = nullptr;
    MTL::Device* device = nullptr;
};
```
#### Residency Manager
Asset loading should be done on a separate thread from rendering. Residency sets are updated in parallel with encoding.
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
    
    private:
    MTL::ResidencySet* persistentSet = nullptr;
    MTL::ResidencySet* dynamicSet = nullptr;
    
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

## Drawing
The draw function connects the systems together to actually draw the scene.

### Drawing Structs and Classes
#### Draw Submesh Command Descriptor
```cpp
using DrawSortKey = uint64_t;

struct DrawSubmeshCommandDescriptor
{
    SubmeshHandle submesh;
    uint32_t resolvedTransformationIndex;
    AnimationInstanceHandle animationInstance;
    MaterialHandle material;
    
    /**
     * The sort key of the descriptor.
     * @param staticSortKeyParts Precalculated bits for the pipeline flags, Material ID, and Submesh ID.
     */
    DrawSortKey sortKey(uint64_t staticSortKeyParts = UINT64_MAX) const
    {
        /**
         * The material ID, Submesh ID, and pipeline flags may be precalculated.
         * 
         * If opaque:
         *     First two bits: 00
         *     Next 3 bits: Pipeline Flags
         *     Next 16 bits: Material ID
         *     Next 22 bits: Submesh ID
         *     Last 21 bits: Reserved (depth)
         * 
         * If translucent:
         *     First two bits: 01
         *     Next 21 bits: Reserved (depth)
         *     Next 3 bits: Pipeline Flags
         *     Next 16 bits: Material ID
         *     Last 22 bits: Submesh ID
         */
    }
};
```

#### Render Queue
Lines up objects in the order that they should be drawn in.
```cpp
class RenderQueue
{
    public:
    
    /**
     * Builds an unsorted render queue.
     * @param sceneObjectSystem
     * @param transformationSystem
     * @param cullingSystem
     * @param camera
     */
    void build(
        const SceneObjectSystem& sceneObjectSystem, 
        const TransformationSystem& transformationSystem, 
        const CullingSystem& cullingSystem, 
        const Camera& camera);
    
    /**
     * Adds a scene object to the render queue.
     * More specifically, builds a `DrawSubmeshCommandDescriptor` using the information in the scene object handle.
     * @param sceneObjectHandle The handle of the scene object to add to the render queue
     * @param sceneObjectSystem
     * @param camera
     * @param transformationSystem
     */
    void add(
        SceneObjectHandle sceneObjectHandle, 
        const SceneObjectSystem& sceneObjectSystem, 
        const Camera& camera,
        const TransformationSystem& transformationSystem);
    
    /**
     * Clears the render queue, setting size to 0.
     */
    void clear();
    
    /**
     * Sorts the render queue based on the Draw Sort Keys of each `DrawSubmeshCommandDescriptor`.
     */
    void sort();

    std::vector<DrawSubmeshCommandDescriptor> queue;
};
```

##### Sorting the Render Queue
Opaque objects should be drawn from front to back to minimize overdraw. Translucent objects need to be drawn from back to front to ensure proper alpha blending (for example, red colored glass applying a red tint to the objects behind it). 21 bits have been reserved for sorting by depth.

Bit packing for opaque materials:

| 2 bits       | 3 bits         | 16 bits     | 22 bits    | 21 bits  |
| ------------ | -------------- | ----------- | ---------- | -------- |
| Translucency | pipeline flags | Material ID | Submesh ID | Reserved |

Bit packing for translucent materials (higher priority of bits reserved for depth):

| 2 bits       | 21 bits  | 3 bits         | 16 bits     | 22 bits    |
| ------------ | -------- | -------------- | ----------- | ---------- |
| Translucency | Reserved | pipeline flags | Material ID | Submesh ID |

These keys can then be sorted using radix sort, giving us $O(d \cdot n)$ worst case performance and $O(d + n)$ worst case space complexity.

### Draw Function

Each frame:
- Flush [deletion queue](#deletion-queue)
- Flush addition queues
- The command allocator pool begins a frame
- [TransformationSystem](#transformation-system) copies world matrices into buffer
- [AnimationSystem](#animation-system) calculates skinning matrices and then puts them into `skinningBuffer`
- Perform frustum culling
- Clear the [render queue](#render-queue)
- Build the render queue
	- For each scene object:
		- If an object is visible (determined through frustum culling operation):
            - Create a `DrawSubmeshCommandDescriptor` from the information in the [Scene Object System](#scene-object-system)
            - Add the `DrawSubmeshCommandDescriptor` to the render queue.
    - [Sort the render queue](#sorting-the-render-queue) by sortKey.
- Update render pass descriptor
- Create Render command encoder from command buffer
- Configure the render command encoder (involves setting depth stencil state)
- Put view matrix into vertex bytes
- Put perspective matrix into vertex bytes
- For each `DrawSubmeshCommandDescriptor` in the render queue:
    - If the pipeline flags do not match the current pipeline flags, switch the pipeline state.
    - If translucency flag of material handle changes, switch depth stencil state
    - Update argument table:
        - Transformations buffer
        - Textures for the material
        - If the animation instance handle is not invalid, the skinning buffer
    - Apply argument tables (vertex table, fragment table) to the render command encoder
    - Draw indexed primitives using the index buffer.
- End encoding and release render command encoder
- Commit the command buffer
- Signal the drawable that the GPU is done with the render pass
- Present the drawable

```cpp
void DecoEngine::draw(CA::MetalDrawable* drawable)
{
    flushDeletionQueue(frameNumber);
    
    // Beginning a frame
    commandAllocatorPool.beginFrame(commandQueue);
    
    // Copying matrices into buffers
    transformationSystem.update();
    animationSystem.update(deltaTime); // deltaTime is time between frames
    transformationSystem.uploadToGPU();
    animationSystem.uploadToGPU();
    
    // Frustum culling
    cullingSystem.cull();
    
    // Render Queue
    renderQueue.clear();
    renderQueue.build(
        sceneObjectSystem, 
        transformationSystem, 
        cullingSystem, 
        camera);
    renderQueue.sort();
    
    // Update the render pass descriptor
    updateRenderPassDescriptor(drawable);
    
    MTL4::CommandBuffer* commandBuffer = commandAllocatorPool.getCommandBuffer();
    MTL4::RenderCommandEncoder* renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
    
    configureRenderCommandEncoder(renderCommandEncoder);
    
    // Put view and perspective matrices into vertex bytes
    encodeViewMatrix(camera.getViewMatrix(transformationSystem), renderCommandEncoder);
    encodePerspectiveMatrix(camera.getPerspectiveMatrix(), renderCommandEncoder);
    
    // Drawing
    drawObjectsInRenderQueue();
    
    // End encoding
    renderCommandEncoder->endEncoding();
    renderCommandEncoder->release();
    
    // Wait for residency set to commit
    residencyManager.waitForCommit();
    
    // Committing the command buffer
    commandQueue->commit(commandBuffer);
    
    // Signaling the drawable that the GPU is done with the render pass
    commandQueue->signalDrawable(drawable);
    
    // Presenting the drawable
    drawable->present();
}
```

#### Potential Future Drawing Optimizations
- Not calculating skinning matrices for animation instances outside of frustum
- Frustum culling split into passes of increasing precision.

## Loading Assets
Loading assets is done on a thread that is separate from the rendering thread. Model loading is primarily handled through the ufbx library. Once a model has been loaded into memory, the residency sets are updated, then committed.

*See [Asset Loading](./asset_loading.md)*

### Loading Models
When requesting that a model be loaded in, the caller passes in the filepath to a `.fbx` file, as well as a callback function that is to be called once loading has finished.

During loading, items are queued up to be inserted into different systems, with handles being assigned based on listed free handles and the number of entries in each system.

When a model is loaded:
- queue up the vertex buffer, index buffer, and index count for each submesh in the model (queuing up a submesh) for the submesh system
- queue up the skeleton of the model for the animation system
- queue up animation clips for the animation system
- queue up any materials from textures that may be in the fbx file for the material system
- At the start of each frame:
    - For every submesh in the queue:
        - Add the vertex buffer, index buffer, and index count to the arrays in the submesh system
    - For every material in the queue:
        - add the materials to the material array
        - put the skeleton in the queue into the skeletons array of the animation system
    - For every clip in the queue:
        - Add the clip to the clips array in the animation system
    

When a model is added to the scene
- Create an animation instance using the skeleton
- Create the transformations for each submesh in the model based on information loaded in
- Pass in the submesh handles, material handles, animation instance handles, and transformation handles

## Unloading Assets
Unloading models is done on a thread that is separate from the rendering thread. Because unloading a model involves making it no longer resident, the residency manager will halt execution of the rendering thread until the update to the residency set has been committed.

### Deletion Queue
When items are unloaded, they should be added to a deletion queue with the specification that they are safe to delete at frame $i + k$, where $i$ is the current frame number and $k$ is the maximum number of frames in flight (3 by default).

```cpp
struct DeletionQueueEntry
{
    // The frame that it is safe to delete the object after
    uint64_t safeToDeleteAfterFrame;

    MTL::Resource* resource;
};

std::queue<DeletionQueueEntry> deletionQueue;

/**
 * Deletes items at the front of the deletion queue that are safe
 * to delete in the specified frame.
 */
void flushDeletionQueue(uint64_t frame);
```
