#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H


#include "scene/3d/mesh_instance_3d.h"

#include "modules/noise/fastnoise_lite.h"


class MeshInstance3D;
class Node3D;
class Curve;
class Texture;
class FastNoiseLite;
struct Vector2;
class Chunk;

constexpr int min_octaves = 1;
constexpr int max_octaves = 10;

class ProceduralTerrain : public Node3D {
    GDCLASS(ProceduralTerrain, Node3D);
    
    int octaves;
    real_t lacunarity;
    real_t persistence;
    Ref<FastNoiseLite> noise;
    real_t height_scale;
    Ref<Curve> height_curve;
    NodePath observer;
    Array visible_chunks;
    Dictionary generated_chunks;
    real_t view_distance;
    PackedFloat32Array view_thresholds;
    bool reset_chunks_on_change;
    Vector2 falloff;
    
public:
    void set_octaves(const int p_octaves) { octaves = CLAMP(p_octaves, min_octaves, max_octaves); clear_chunks(); };
    int get_octaves() const { return octaves; }

    void set_lacunarity(const real_t p_lacunarity) { lacunarity = p_lacunarity; clear_chunks(); }
    real_t get_lacunarity() const { return lacunarity; }

    void set_persistence(const real_t p_persistence) { persistence = p_persistence; }
    real_t get_persistence() const { return persistence; }

    void set_height_scale(const real_t p_height_scale) { height_scale = p_height_scale; clear_chunks(); }
    real_t get_height_scale() const { return height_scale; }

    void set_noise(const Ref<FastNoiseLite> &p_noise) { noise = p_noise; clear_chunks(); }
    Ref<FastNoiseLite> get_noise() const { return noise; }

    void set_height_curve(const Ref<Curve> &p_height_curve) { height_curve = p_height_curve; clear_chunks(); }
    Ref<Curve> get_height_curve() const { return height_curve; };

    void set_observer(const NodePath& p_observer) { observer = p_observer; clear_chunks(); }
    NodePath get_observer() const { return observer; }

    void set_view_distance(const real_t p_view_distance) { view_distance = p_view_distance; }
    real_t get_view_distance() const { return view_distance; }

    void set_view_thresholds(PackedFloat32Array p_view_thresholds);
    PackedFloat32Array get_view_thresholds() const { return view_thresholds; }
    
    void clear_chunks();

    void set_falloff(const Vector2 p_falloff) { falloff = p_falloff; }
    Vector2 get_falloff() const { return falloff; }
    
    static Ref<Mesh> generate_chunk(const Ref<FastNoiseLite>& noise, const Ref<Curve>& height_curve, int level_of_detail,
        const Ref<StandardMaterial3D>& material, int octaves, real_t persistence, real_t lacunarity, real_t height_scale,
        Vector2 offset, Vector2 falloff);
    
    ProceduralTerrain();

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    void _update();
    float _get_distance_to_chunk(const Chunk* chunk) const;

    static Array _generate_matrix(int octaves, const Ref<FastNoiseLite>& noise, real_t persistence, real_t lacunarity, Vector2 offset);
    static Ref<ArrayMesh> _generate_mesh(const Array& matrix, int level_of_detail, const Ref<Curve>& height_curve, real_t height_scale);
    static void ProceduralTerrain::_generate_material(const Array& matrix, const Ref<StandardMaterial3D>& material);
    static Array _generate_falloff(Vector2 falloff);
};

#endif