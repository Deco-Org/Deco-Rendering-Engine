# Deco Rendering Engine Architecture

## Layers
The Deco Rendering Engine uses a combination of Object Oriented and Data Oriented Design. Data Oriented Design is used at low levels where efficiency is key (_see [AoS and SoA on Wikipedia](https://en.wikipedia.org/wiki/AoS_and_SoA)_).
### Core Systems
#### Animation System
Uses arrays of structures. There will be very few animated characters in a scene at once, so the ergonomic downsides of using structures as arrays are likely not worth the potential performance gains.
```cpp
namespace Deco {
    using ClipHandle = uint32_t;
    using AnimationInstanceHandle = uint32_t;
    using SkeletonHandle = uint32_t;
    using BoneHandle = uint32_t;
    
    inline constexpr BoneHandle NO_PARENT = UINT32_MAX;
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
        void play(AnimationInstanceHandle instance, ClipHandle clip, bool loop = false);
        void stop(AnimationInstanceHandle instance);
        void update(float deltaTime); // advance all playing instances
        void setPlaybackSpeed(AnimationInstanceHandle instance, float speed);
        float getPlaybackTime(AnimationInstanceHandle instance);
        void uploadToGPU();
    };
}
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