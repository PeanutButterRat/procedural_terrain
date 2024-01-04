#include "chunk.h"


#include "core/core_bind.h"


void Chunk::request_mesh(int level_of_detail) {
    if (resource_threads.has(level_of_detail)) {
        const Ref<core_bind::Thread> thread = resource_threads[level_of_detail];
        if (!thread->is_alive()) {
            meshes[level_of_detail] = thread->wait_to_finish();
            resource_threads.erase(level_of_detail);
        }
    }
    else if (!meshes.has(level_of_detail)) {
        Ref<StandardMaterial3D> material{};
        material.instantiate();
        material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
        materials[level_of_detail] = material;
		
        Ref<core_bind::Thread> thread{};
        thread.instantiate();
        resource_threads[level_of_detail] = thread;
		
        const Node* terrain = get_parent();
        thread->start(Callable{terrain, "generate_chunk"}.bind(
            terrain->get("noise").duplicate(),
            terrain->get("height_curve").duplicate(),
            level_of_detail,
            material,
            terrain->get("octaves"),
            terrain->get("persistence"),
            terrain->get("lacunarity"),
            terrain->get("height_scale"),
            Vector2{get_position().x, get_position().z},
            terrain->get("falloff_parameters")
        ));
    }
	
    if (meshes.has(level_of_detail)) {
        set_mesh(meshes[level_of_detail]);
        set_material_override(materials[level_of_detail]);
    }
}

Chunk::~Chunk() {
    const Array threads = resource_threads.values();
    for (int i = 0; i < threads.size(); i++) {
        cast_to<core_bind::Thread>(threads.get(i))->wait_to_finish();
    }
}