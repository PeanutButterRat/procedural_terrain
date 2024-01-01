﻿#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H


#include "scene/3d/mesh_instance_3d.h"

#include "scene/resources/image_texture.h"


class MeshInstance3D;
class Node3D;
class Curve;
class Texture;
class FastNoiseLite;
struct Vector2;


class Chunk : public MeshInstance3D {
    Dictionary resource_threads;
    Dictionary meshes;
    Dictionary materials;
    
public:
    void request_mesh(int level_of_detail);
    ~Chunk() override;
};

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
    
public:
    void set_octaves(int p_octaves);
    int get_octaves() const;

    void set_lacunarity(real_t p_lacunarity);
    real_t get_lacunarity() const;

    void set_persistence(real_t p_persistence);
    real_t get_persistence() const;

    void set_height_scale(real_t p_height_scale);
    real_t get_height_scale() const;

    void set_noise(const Ref<FastNoiseLite> &p_noise);
    Ref<FastNoiseLite> get_noise() const;

    void set_height_curve(const Ref<Curve> &p_height_curve);
    Ref<Curve> get_height_curve() const;

    void set_observer(const NodePath& p_observer);
    NodePath get_observer() const;

    void set_view_distance(real_t p_view_distance);
    real_t get_view_distance() const;

    void set_view_thresholds(PackedFloat32Array p_view_thresholds);
    PackedFloat32Array get_view_thresholds() const;

    void set_reset_chunks_on_change(bool p_reset_chunks_on_change);
    bool get_reset_chunks_on_change() const;
    
    void reset_chunks();
    
    static Ref<Mesh> generate_chunk(const Ref<FastNoiseLite>& noise, const Ref<Curve>& height_curve, int level_of_detail,
        const Ref<StandardMaterial3D>& material, int octaves, real_t persistence, real_t lacunarity, real_t height_scale, Vector2 offset);
    
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
};

#endif