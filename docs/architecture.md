# Deco Rendering Engine Architecture

## Layers
### Core Systems
#### Animation System
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
