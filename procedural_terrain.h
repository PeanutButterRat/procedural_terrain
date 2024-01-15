#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H

#include "scene/3d/node_3d.h"

#include "procedural_terrain_parameters.h"

class MeshInstance3D;
class StaticBody3D;
class CollisionShape3D;
class Node3D;
class Curve;
class Texture;
class FastNoiseLite;
struct Vector2;


class ProceduralTerrain : public Node3D {
    GDCLASS(ProceduralTerrain, Node3D);

public:
    enum GenerationMode {
        GENERATION_MODE_NORMAL,
        GENERATION_MODE_FALLOFF,
        GENERATION_MODE_NOISE_UNSHADED,
        GENERATION_MODE_NOISE_SHADED,
    };

private:
    GenerationMode mode;
    NodePath viewer;
    PackedInt32Array detail_offsets;
    Ref<ProceduralTerrainParameters> terrain_parameters;
    
    Array visible_chunks;
    Dictionary generated_chunks;
    Dictionary threads;
    
public:
    void set_generation_mode(GenerationMode p_mode) { mode = p_mode; clear_chunks(); }
    GenerationMode get_generation_mode() const { return mode; }
    
    void set_viewer(const NodePath& p_observer) { viewer = p_observer; clear_chunks(); }
    NodePath get_viewer() const { return viewer; }
    
    void set_detail_offsets(PackedInt32Array p_detail_offsets) {
        for (int i = 0; i < p_detail_offsets.size(); i++) {
            p_detail_offsets.set(i, CLAMP(p_detail_offsets[i], MIN_LEVEL_OF_DETAIL, MAX_LEVEL_OF_DETAIL));
        }
        detail_offsets = p_detail_offsets;
    }
    PackedInt32Array get_detail_offsets() const { return detail_offsets; }
    
    void set_terrain_parameters(const  Ref<ProceduralTerrainParameters>& parameters) {
        if (terrain_parameters.is_valid()) { terrain_parameters->disconnect_changed(callable_mp(this, &ProceduralTerrain::clear_chunks)); }
        terrain_parameters = parameters;
        if (terrain_parameters.is_valid()) { terrain_parameters->connect_changed(callable_mp(this, &ProceduralTerrain::clear_chunks)); }
    }
    Ref<ProceduralTerrainParameters> get_terrain_parameters() const { return terrain_parameters; }
    
    static MeshInstance3D* generate_terrain(const Ref<ProceduralTerrainParameters>& parameters, bool collision, GenerationMode mode);
    
    void clear_chunks();
    ProceduralTerrain() { set_process_internal(true); mode = GENERATION_MODE_NORMAL; }
    ~ProceduralTerrain() override { clear_chunks(); }

protected:
    static void _bind_methods();
    void _notification(int p_what) { if (p_what == NOTIFICATION_INTERNAL_PROCESS) _internal_process(); }

private:
    void _internal_process();

    static Array generate_matrix(int octaves, const Ref<FastNoiseLite>& noise, real_t persistence, real_t lacunarity);
    static Ref<Mesh> generate_mesh(const Array& matrix, int level_of_detail, const Ref<Curve>& height_curve, real_t height_scale);
    static Ref<Mesh> generate_flatshaded_mesh(const Array& matrix, int level_of_detail, const Ref<Curve>& height_curve, real_t height_scale);
    static Ref<StandardMaterial3D> generate_material(const Array& matrix, const Ref<Gradient>& color_map);
    static Array generate_falloff(Vector2 falloff);
    static void apply_falloff(Array matrix, const Array& falloff);
    static StaticBody3D* generate_collision(const Ref<Mesh>& mesh);
};

VARIANT_ENUM_CAST(ProceduralTerrain::GenerationMode);

#endif