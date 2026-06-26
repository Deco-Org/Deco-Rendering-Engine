#include "mesh.hpp"
#include <stdio.h>

MTL::Device *Mesh::metalDevice = nullptr;

Mesh::Mesh(MTL::Device *device)
{
    metalDevice = device;
}

Mesh::~Mesh()
{

}

void Mesh::setMetalDevice(MTL::Device* device)
{
    metalDevice = device;
}

MeshAsset* Mesh::loadModel(const char *path)
{
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file(path, &opts, &error);
    if (!scene) {
        fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
        return;
    }

    ufbx_node *node = scene->nodes.data[0];

    node = scene->nodes.data[1];

    MeshData meshData;

    for (ufbx_mesh *mesh : scene->meshes)
    {
        if (!mesh)
        {
            continue;
        }
        printf("Mesh is %s\n", mesh->name.data);
        meshData = MeshData(getDataForBuffer(mesh));
    }

    ufbx_free_scene(scene);

    // TODO: At some point, we need to handle the file not being found, or the mesh data being empty

    // Putting data into mesh asset
    MeshAsset* asset = new MeshAsset;
    asset->vertexBuffer = metalDevice->newBuffer(
        meshData.vertices.data(),
        sizeof(VertexData) * meshData.vertices.size(),
        MTL::ResourceStorageModeShared
    );
    asset->indexBuffer = metalDevice->newBuffer(
        meshData.indices.data(),
        sizeof(uint32_t) * meshData.indices.size(),
        MTL::ResourceStorageModeShared
    );
    asset->indexCount = meshData.indices.size();
    return asset;
}

MeshData Mesh::getDataForBuffer(ufbx_mesh* mesh)
{
    MeshData meshData = {
        .vertices = std::vector<VertexData>(),
        .indices = std::vector<uint32_t>()
    };
    std::vector<VertexData>* vertices = &meshData.vertices;
    std::vector<uint32_t> tri_indices = std::vector<uint32_t>();
    // For now, we'll just get this first material
    // ufbx_mesh_part& part = mesh->materials.data[0];
    ufbx_mesh_part& part = mesh->material_parts.data[0];
    tri_indices.resize(mesh->max_face_triangles * 3);

    for (uint32_t face_index : part.face_indices)
    {
        ufbx_face face = mesh->faces[face_index];

        // Triangulating the face into indices[]
        uint32_t number_of_triangles = ufbx_triangulate_face(
            tri_indices.data(), tri_indices.size(), mesh, face);

        // Iterating over each triangle corner
        for (size_t i = 0; i < number_of_triangles * 3; i++)
        {
            uint32_t index = tri_indices[i];

            VertexData v;
            v.position = (simd::float3) {
                mesh->vertex_position[index].x,
                mesh->vertex_position[index].y,
                mesh->vertex_position[index].z
            };

            v.normal = (simd::float3) {
                mesh->vertex_normal[index].x,
                mesh->vertex_normal[index].y,
                mesh->vertex_normal[index].z
            };

            v.textureCoordinate = (simd::float2) {
                mesh->vertex_uv[index].x,
                mesh->vertex_uv[index].y
            };

            vertices->push_back(v);
        }
    }

    // Making sure that all vertices have been written
    assert(vertices->size() == part.num_triangles * 3);

    // Generating index buffer
    ufbx_vertex_stream streams[1] = {
        { vertices->data(), vertices->size(), sizeof(VertexData) },
    };

    meshData.indices.resize(part.num_triangles * 3);

    // Deduplicating vertices and writing indices to `indices[]`
    size_t num_vertices = ufbx_generate_indices(
        streams, 1, meshData.indices.data(), meshData.indices.size(), nullptr, nullptr);

    // Trimming the vertices vector to hold only unique vertices
    vertices->resize(num_vertices);

    return meshData;
}