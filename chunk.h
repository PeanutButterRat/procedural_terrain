#ifndef CHUNK_H
#define CHUNK_H


#include "scene/3d/mesh_instance_3d.h"


class Chunk : public MeshInstance3D {
    Dictionary resource_threads;
    Dictionary meshes;
    Dictionary materials;
    
public:
    void request_mesh(int level_of_detail);
    ~Chunk() override;
};

#endif