#ifndef PROCEDURAL_TERRAIN_PARAMETERS_H
#define PROCEDURAL_TERRAIN_PARAMETERS_H


#include "modules/noise/fastnoise_lite.h"

#include "scene/resources/curve.h"

constexpr int min_octaves = 1;
constexpr int max_octaves = 10;
constexpr int min_level_of_detail = 0;
constexpr int max_level_of_detail = 6;


class ProceduralTerrainParameters : public Resource {
    GDCLASS(ProceduralTerrainParameters, Resource);

    Ref<FastNoiseLite> noise;
    Ref<Curve> height_curve;
    int octaves;
    int level_of_detail;
    real_t lacunarity;
    real_t persistence;
    real_t height_scale;
    Vector2 offset;
    Vector2 falloff;
    
public:
    ProceduralTerrainParameters() {
        octaves = min_octaves;
        level_of_detail = max_level_of_detail;
        lacunarity = 1.0f;
        persistence = 1.0f;
        height_scale = 10.0f;
    };
    
    void set_noise(const Ref<FastNoiseLite> &p_noise) {
        if (noise.is_valid()) {
            noise->disconnect_changed(callable_mp(this, &ProceduralTerrainParameters::_update));
        }

        noise = p_noise;

        if (noise.is_valid()) {
            noise->connect_changed(callable_mp(this, &ProceduralTerrainParameters::_update));
        }
        
        _update();
    }
    Ref<FastNoiseLite> get_noise() const { return noise; }
    
    void set_height_curve(const Ref<Curve> &p_height_curve) {
        if (height_curve.is_valid()) {
            height_curve->disconnect_changed(callable_mp(this, &ProceduralTerrainParameters::_update));
        }

        height_curve = p_height_curve;

        if (height_curve.is_valid()) {
            height_curve->connect_changed(callable_mp(this, &ProceduralTerrainParameters::_update));
        }
        
        _update();
    }
    Ref<Curve> get_height_curve() const { return height_curve; };
    
    void set_octaves(const int p_octaves) {
        if (p_octaves > max_octaves || p_octaves < min_octaves) {
            WARN_PRINT(String("Octaves must be within the range of") + min_octaves + " and " + max_octaves + ", value will be clamped.");
        }
        octaves = CLAMP(p_octaves, min_octaves, max_octaves);
        _update();
    }
    int get_octaves() const { return octaves; }

    void set_level_of_detail(const int p_level_of_detail) { level_of_detail = p_level_of_detail; _update(); }
    int get_level_of_detail() const { return level_of_detail; }
    
    void set_lacunarity(const real_t p_lacunarity) { lacunarity = p_lacunarity; _update(); }
    real_t get_lacunarity() const { return lacunarity; }

    void set_persistence(const real_t p_persistence) { persistence = p_persistence; _update(); }
    real_t get_persistence() const { return persistence; }

    void set_height_scale(const real_t p_height_scale) { height_scale = p_height_scale; _update(); }
    real_t get_height_scale() const { return height_scale; }
    
    void set_falloff(const Vector2 p_falloff) { falloff = p_falloff; _update(); }
    Vector2 get_falloff() const { return falloff; }

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &ProceduralTerrainParameters::set_octaves);
		ClassDB::bind_method(D_METHOD("get_octaves"), &ProceduralTerrainParameters::get_octaves);

		ClassDB::bind_method(D_METHOD("set_lacunarity", "lacunarity"), &ProceduralTerrainParameters::set_lacunarity);
		ClassDB::bind_method(D_METHOD("get_lacunarity"), &ProceduralTerrainParameters::get_lacunarity);

		ClassDB::bind_method(D_METHOD("set_persistence", "persistence"), &ProceduralTerrainParameters::set_persistence);
		ClassDB::bind_method(D_METHOD("get_persistence"), &ProceduralTerrainParameters::get_persistence);
		
		ClassDB::bind_method(D_METHOD("set_height_scale", "scale"), &ProceduralTerrainParameters::set_height_scale);
		ClassDB::bind_method(D_METHOD("get_height_scale"), &ProceduralTerrainParameters::get_height_scale);

		ClassDB::bind_method(D_METHOD("set_noise", "noise"), &ProceduralTerrainParameters::set_noise);
		ClassDB::bind_method(D_METHOD("get_noise"), &ProceduralTerrainParameters::get_noise);

		ClassDB::bind_method(D_METHOD("set_height_curve", "curve"), &ProceduralTerrainParameters::set_height_curve);
		ClassDB::bind_method(D_METHOD("get_height_curve"), &ProceduralTerrainParameters::get_height_curve);
        
		ClassDB::bind_method(D_METHOD("set_falloff", "falloff"), &ProceduralTerrainParameters::set_falloff);
		ClassDB::bind_method(D_METHOD("get_falloff"), &ProceduralTerrainParameters::get_falloff);
        
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_height_curve", "get_height_curve");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves", PROPERTY_HINT_RANGE, itos(min_octaves) + "," + itos(max_octaves) + ",1"), "set_octaves", "get_octaves");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lacunarity"), "set_lacunarity", "get_lacunarity");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "persistence"), "set_persistence", "get_persistence");
        
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale"), "set_height_scale", "get_height_scale");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "falloff"), "set_falloff", "get_falloff");

        BIND_CONSTANT(min_level_of_detail);
        BIND_CONSTANT(max_level_of_detail);
        BIND_CONSTANT(min_octaves);
        BIND_CONSTANT(max_octaves);
    };

private:
    void _update() { emit_changed(); }
};

#endif
