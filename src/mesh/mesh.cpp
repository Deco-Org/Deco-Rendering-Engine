#include "mesh.hpp"
#include <stdio.h>
#include "ufbx.h"

Mesh::Mesh()
{
    ufbx_load_opts opts = { };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file("assets/test_cube.fbx", &opts, &error);
    if (!scene) {
        fprintf(stderr, "Failed to load scene: %s\n", error.description.data);
        return;
    }

    for (ufbx_node *node : scene->nodes) {
        printf("%s\n", node->name.data);
    }

    ufbx_free_scene(scene);
}

Mesh::~Mesh()
{

}

void Mesh::loadModel(char *path)
{

}