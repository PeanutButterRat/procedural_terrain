#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H


#include "procedural_terrain_parameters.h"
#include "scene/3d/node_3d.h"

class MeshInstance3D;
class Node3D;
class Curve;
class Texture;
class FastNoiseLite;
struct Vector2;
class ProceduralTerrainChunk;
class ProceduralTerrainParameters;


class ProceduralTerrain : public Node3D {
    GDCLASS(ProceduralTerrain, Node3D);
    
    NodePath viewer;
    PackedFloat32Array view_thresholds;
    Ref<ProceduralTerrainParameters> terrain_parameters;
    
    Array visible_chunks;
    Dictionary generated_chunks;

    
public:
    void set_viewer(const NodePath& p_observer) { viewer = p_observer; clear_chunks(); }
    NodePath get_viewer() const { return viewer; }
    
    void set_view_thresholds(PackedFloat32Array p_view_thresholds);
    PackedFloat32Array get_view_thresholds() const { return view_thresholds; }

    void set_terrain_parameters(const  Ref<ProceduralTerrainParameters>& parameters) {
        if (terrain_parameters.is_valid()) {
            terrain_parameters->disconnect_changed(callable_mp(this, &ProceduralTerrain::clear_chunks));
        }
        
        terrain_parameters = parameters;

        if (terrain_parameters.is_valid()) {
            terrain_parameters->connect_changed(callable_mp(this, &ProceduralTerrain::clear_chunks));
        }
    }
    Ref<ProceduralTerrainParameters> get_terrain_parameters() const { return terrain_parameters; }
    
    void clear_chunks();
    
    static Ref<Mesh> generate_terrain(const Ref<ProceduralTerrainParameters>& parameters, const Ref<StandardMaterial3D>& material);
    
    ProceduralTerrain();

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    void _update();
    float _get_distance_to_chunk(const ProceduralTerrainChunk* chunk) const;

    static Array _generate_matrix(int octaves, const Ref<FastNoiseLite>& noise, real_t persistence, real_t lacunarity);
    static Ref<ArrayMesh> _generate_mesh(const Array& matrix, int level_of_detail, const Ref<Curve>& height_curve, real_t height_scale);
    static void ProceduralTerrain::_generate_material(const Array& matrix, const Ref<StandardMaterial3D>& material);
    static Array _generate_falloff(Vector2 falloff);
};

#endif