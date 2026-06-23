#pragma once
#include "vertex_data.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"
#include <Metal/Metal.hpp>
#include <vector>

// Mostly kept around for easy testing
struct MeshData
{
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};

struct MeshAsset
{
    MTL::Buffer* vertexBuffer;
    MTL::Buffer* indexBuffer;
    uint32_t indexCount;
};

class Mesh
{
    public:
    Mesh(MTL::Device *device = nullptr);
    ~Mesh();

    static MeshAsset* loadModel(const char* filePath);
    
    static void setMetalDevice(MTL::Device* metalDevice);

    private:
    static MeshData getDataForBuffer(ufbx_mesh* mesh);

    static MTL::Device* metalDevice;
};