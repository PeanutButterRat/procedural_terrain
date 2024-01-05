#ifndef CHUNK_H
#define CHUNK_H


#include "scene/3d/mesh_instance_3d.h"
#include "core/core_bind.h"

#include "procedural_terrain.h"
#include "procedural_terrain_parameters.h"


class ProceduralTerrainParameters;

class ProceduralTerrainChunk : public MeshInstance3D {
    Dictionary meshes;
    Dictionary threads;
    Dictionary materials;

public:
    ProceduralTerrainChunk::~ProceduralTerrainChunk() override {
        const Array values = threads.values();
        for (int i = 0; i < values.size(); i++) {
            cast_to<core_bind::Thread>(values.get(i))->wait_to_finish();
        }
    }
    
    void request_mesh(const int level_of_detail) {
        if (threads.has(level_of_detail)) {
            const Ref<core_bind::Thread> thread = threads[level_of_detail];
            if (!thread->is_alive()) {
                meshes[level_of_detail] = thread->wait_to_finish();
                threads.erase(level_of_detail);
            }
        }
        else if (!meshes.has(level_of_detail)) {
            Ref<StandardMaterial3D> material{};
            material.instantiate();
            material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
            materials[level_of_detail] = material;
		
            Ref<core_bind::Thread> thread{};
            thread.instantiate();
            threads[level_of_detail] = thread;
		
            const ProceduralTerrain* terrain = cast_to<ProceduralTerrain>(get_parent());
            const Ref<ProceduralTerrainParameters> parameters = terrain->get_terrain_parameters()->duplicate(true);
            const Ref<FastNoiseLite> noise = parameters->get_noise();
            noise->set_offset(noise->get_offset() + Vector3{get_position().z, -get_position().x, 0.0f});
            parameters->set_level_of_detail(level_of_detail);
            
            thread->start(Callable{terrain, "generate_terrain"}.bind(parameters, material));
        }
	
        if (meshes.has(level_of_detail)) {
            set_mesh(meshes[level_of_detail]);
            set_material_override(materials[level_of_detail]);
        }
    }
};

#endif