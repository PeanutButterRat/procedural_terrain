#include "register_types.h"


#include "core/object/class_db.h"

#include "procedural_terrain.h"
#include "procedural_terrain_parameters.h"


void initialize_procedural_terrain_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    
    ClassDB::register_class<ProceduralTerrain>();
    ClassDB::register_class<ProceduralTerrainParameters>();
}

void uninitialize_procedural_terrain_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}