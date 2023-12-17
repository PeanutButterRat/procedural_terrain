#include "register_types.h"

#include "core/object/class_db.h"
#include "procedural_terrain.h"

void initialize_procedural_terrain_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    
    ClassDB::register_class<ProceduralTerrain>();
}

void uninitialize_procedural_terrain_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}