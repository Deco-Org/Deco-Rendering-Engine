# Asset Loading
## Loading Models
When requesting that a model be loaded in, the caller passes in the filepath to a `.fbx` file, as well as a callback function that is to be called once loading has finished.

During loading, items are queued up to be inserted into different systems, with handles being assigned based on listed free handles and the number of entries in each system.

```mermaid
sequenceDiagram

actor caller as Caller
participant loadingThread as Loading Thread
participant renderThread as Render Thread

caller ->> loadingThread: load request
loadingThread -->> caller: Asset Handle
loadingThread ->> loadingThread: Parse fbx file using ufbx
loadingThread ->> loadingThread: Create GPU buffers for meshes, textures, and skinning matrices

loadingThread ->> renderThread: Push buffers to queues
renderThread ->> renderThread: Drain queues
renderThread -->> loadingThread: Fill 'used handles' array
loadingThread ->> loadingThread: Update list of free handles
loadingThread -->> caller: Loading complete callback
```

```mermaid
sequenceDiagram

box Loading Thread
    actor caller as Requster
    participant assetLoader as Asset Loader
    participant submeshSystem as Submesh System
    participant materialSystem as Material System
    participant animationSystem as Animation System
end
box Render Thread
    participant submeshSystemRenderT as Submesh System
    participant materialSystemRenderT as Material System
    participant animationSystemRenderT as Animation System
end

caller ->> assetLoader: Request asset loaded from .fbx file
assetLoader ->> assetLoader: Parse file using ufbx

assetLoader ->> submeshSystem: Load meshes from a list of ufbx_mesh
submeshSystem ->> submeshSystem: Get material_parts for ufbx_mesh.
submeshSystem ->> submeshSystem: Parse each ufbx_mesh_part (submesh) in the material_parts list, create buffers, submesh system info
submeshSystem ->> submeshSystemRenderT: queue up submeshes to be added to submesh system
loop While queue is not empty
    submeshSystemRenderT ->> submeshSystemRenderT: Add submeshes to submesh system
    submeshSystemRenderT ->> submeshSystem: Add added handles to array of used handles
    submeshSystem ->> submeshSystem: Update list of free submesh handles
end
submeshSystem -->> assetLoader: List of submesh handles

assetLoader ->> materialSystem: Load materials from a list of materials in model
materialSystem ->> materialSystemRenderT: queue up materials to be added to material system
loop While queue is not empty
    materialSystemRenderT ->> materialSystemRenderT: Add materials to material system
    materialSystemRenderT ->> materialSystem: Add added handles to array of used handles
    materialSystem ->> materialSystem: Update list of free material handles
end
materialSystem -->> assetLoader: List of material handles

assetLoader ->> animationSystem: Load skeletons
assetLoader ->> animationSystem: Load animation clips
animationSystem ->> animationSystemRenderT: queue up skeletons to be added to animation system
loop While queue is not empty
    animationSystemRenderT ->> animationSystemRenderT: Add skeletons to animation system
    animationSystemRenderT ->> animationSystem: Add added handles to array of used handles
    animationSystem ->> animationSystem: Update list of free skeleton handles
end
animationSystem -->> assetLoader: List of skeleton handles
animationSystem ->> animationSystemRenderT: queue up animation clips to be added to animation system
loop While queue is not empty
    animationSystemRenderT ->> animationSystemRenderT: Add animation clips to animation system
    animationSystemRenderT ->> animationSystem: Add added handles to array of used handles
    animationSystem ->> animationSystem: Update list of free clip handles
end
animationSystem -->> assetLoader: List of animation clip handles

assetLoader -->> caller: Return loaded meshes, materials, skeletons, and animation clips
```

```cpp
using LoadedModelHandle = uint16_t;
using LoadedMeshHandle = uint16_t;

struct LoadedMeshInfo
{
    SubmeshHandle* submeshes;
    MaterialHandle* materials;
    MaterialType* materialTypes;
    size_t submeshCount;
    size_t materialCount;
};

struct LoadedSkeletonsInfo
{
    SkeletonHandle* skeletons;
    size_t skeletonCount;
};

struct LoadedAnimationClipsInfo
{
    AnimationClip* animationClips;
    size_t clipCount;
};

struct LoadedModelInfo
{
    LoadedMeshInfo* meshes;
    LoadedSkeletonsInfo skeletons;
    LoadedAnimationClipsInfo animationClips;
    size_t meshCount;
};
```
When removing a skeleton, the animation clips that use them must be removed first.
```cpp
class ModelLoadingSystem
{
    public:
    ModelLoadingSystem(TextureLoader& textureLoader);
    
    LoadedModelHandle load(std::filesystem::path filepath, void* callback);
    LoadedModelHandle add(LoadedModelInfo modelInfo);
    void removeLoadedModel(LoadedModelHandle model, void* callback);
    void removeAnimationClip(AnimationClipHandle animationClip, void* callback);
    void removeSkeleton(SkeletonHandle skeleton, void* callback);
    void removeMaterial(MaterialHandle material, void* callback);
    void removeLoadedMesh(LoadedMeshHandle mesh, void* callback);
}
```

### Loading Materials From .fbx Files
When a material is stored in an fbx file, the textures associated with the material are loaded in using the `TextureLoader` class. Before they are loaded in, a "dummy material" is used in their place. 

Once the number of loaded items using a texture reaches 0, the texture will be unloaded.
```cpp
struct TrackedTexture
{
    MTL::Texture* texture;
    uint32_t useCount;
};

class TextureLoader
{
    public:
    MTL::Texture* loadTexture(std::filesystem::path filepath);
    void unloadTexture(MTL::Texture* texture);
    void forceUnloadTexture(MTL::Texture* texture);

    private:
    std::unordered_map<std::filesystem::path, TrackedTexture> fileToTextureMap;
};
```