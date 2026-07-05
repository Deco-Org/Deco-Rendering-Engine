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
    participant assetLoader as Asset Loader
    participant meshSystem as Mesh System
    participant materialSystem as Material System
    participant animationSystem as Animation System
end
box Render Thread
    participant meshSystemRenderT as Mesh System
    participant materialSystemRenderT as Material System
    participant animationSystemRenderT as Animation System
end

assetLoader ->> assetLoader: Parse file using ufbx

assetLoader ->> meshSystem: Load meshes from a list of ufbx_mesh
meshSystem ->> meshSystem: Parse ufbx_mesh, create buffers, mesh system info
meshSystem ->> meshSystemRenderT: queue up meshes to be added to mesh system

assetLoader ->> materialSystem: Load materials from a list of materials in model
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