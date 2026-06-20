#pragma once
#include "vertex_data.hpp"
#define UFBX_REAL_IS_FLOAT 1
#include "ufbx.h"
#include <vector>

struct MeshData
{
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};

class Mesh
{
    public:
    Mesh();
    ~Mesh();

    static MeshData* loadModel(char* filePath);

    private:
    static MeshData getDataForBuffer(ufbx_mesh* mesh);
};