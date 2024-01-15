#ifndef PROCEDURAL_TERRAIN_PARAMETERS_H
#define PROCEDURAL_TERRAIN_PARAMETERS_H


#include "modules/noise/fastnoise_lite.h"

#include "scene/resources/curve.h"

constexpr int MIN_OCTAVES = 1;
constexpr int MAX_OCTAVES = 10;
constexpr int MIN_LEVEL_OF_DETAIL = 0;
constexpr int MAX_LEVEL_OF_DETAIL = 6;


class ProceduralTerrainParameters : public Resource {
    GDCLASS(ProceduralTerrainParameters, Resource);

public:
    enum GenerationMode {
        GENERATION_MODE_NORMAL,
        GENERATION_MODE_FALLOFF,
        GENERATION_MODE_NOISE_UNSHADED,
        GENERATION_MODE_NOISE_SHADED,
    };
    
private:
    GenerationMode generation_mode;
    Ref<FastNoiseLite> noise;
    Ref<Curve> height_curve;
    Ref<Gradient> color_map;
    int octaves;
    int level_of_detail;
    real_t lacunarity;
    real_t persistence;
    real_t height_scale;
    Vector2 falloff;
    bool flatshaded;
    
public:
    ProceduralTerrainParameters() {
        generation_mode = GENERATION_MODE_NORMAL;
        octaves = MIN_OCTAVES;
        level_of_detail = MAX_LEVEL_OF_DETAIL;
        lacunarity = 1.0f;
        persistence = 1.0f;
        height_scale = 10.0f;
        flatshaded = false;
    };

    void set_generation_mode(GenerationMode mode) { generation_mode = mode; update(); }
    GenerationMode get_generation_mode() const { return generation_mode; }
    
    void set_noise(const Ref<FastNoiseLite> &p_noise) {
        if (noise.is_valid()) { noise->disconnect_changed(callable_mp(this, &ProceduralTerrainParameters::update)); }
        noise = p_noise;
        if (noise.is_valid()) { noise->connect_changed(callable_mp(this, &ProceduralTerrainParameters::update)); }
        
        update();
    }
    Ref<FastNoiseLite> get_noise() const { return noise; }
    
    void set_height_curve(const Ref<Curve> &p_height_curve) {
        if (height_curve.is_valid()) { height_curve->disconnect_changed(callable_mp(this, &ProceduralTerrainParameters::update)); }
        height_curve = p_height_curve;
        if (height_curve.is_valid()) { height_curve->connect_changed(callable_mp(this, &ProceduralTerrainParameters::update)); }
        
        update();
    }
    Ref<Curve> get_height_curve() const { return height_curve; };

    void set_color_map(const Ref<Gradient> &p_color_map) {
        if (color_map.is_valid()) { color_map->disconnect_changed(callable_mp(this, &ProceduralTerrainParameters::update)); }
        color_map = p_color_map;
        if (color_map.is_valid()) { color_map->connect_changed(callable_mp(this, &ProceduralTerrainParameters::update));}
        
        update();
    }
    Ref<Gradient> get_color_map() const { return color_map; }
    
    void set_octaves(const int p_octaves) {
        if (p_octaves > MAX_OCTAVES || p_octaves < MIN_OCTAVES) {
            WARN_PRINT(String("Octaves must be within the range of") + MIN_OCTAVES + " and " + MAX_OCTAVES + ", value will be clamped.");
        }
        octaves = CLAMP(p_octaves, MIN_OCTAVES, MAX_OCTAVES);
        update();
    }
    int get_octaves() const { return octaves; }

    void set_level_of_detail(const int p_level_of_detail) { level_of_detail = p_level_of_detail; update(); }
    int get_level_of_detail() const { return level_of_detail; }
    
    void set_lacunarity(const real_t p_lacunarity) { lacunarity = p_lacunarity; update(); }
    real_t get_lacunarity() const { return lacunarity; }

    void set_persistence(const real_t p_persistence) { persistence = p_persistence; update(); }
    real_t get_persistence() const { return persistence; }

    void set_height_scale(const real_t p_height_scale) { height_scale = p_height_scale; update(); }
    real_t get_height_scale() const { return height_scale; }
    
    void set_falloff(const Vector2 p_falloff) { falloff = p_falloff; update(); }
    Vector2 get_falloff() const { return falloff; }

    void set_flatshaded(const bool p_flatshaded) { flatshaded = p_flatshaded; update(); }
    bool get_flatshaded() const { return flatshaded; }
    
    bool has_valid_subresources() const { return noise.is_valid() && height_curve.is_valid() && color_map.is_valid(); }

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_generation_mode", "mode"), &ProceduralTerrainParameters::set_generation_mode);
        ClassDB::bind_method(D_METHOD("get_generation_mode"), &ProceduralTerrainParameters::get_generation_mode);
        
		ClassDB::bind_method(D_METHOD("set_noise", "noise"), &ProceduralTerrainParameters::set_noise);
		ClassDB::bind_method(D_METHOD("get_noise"), &ProceduralTerrainParameters::get_noise);

		ClassDB::bind_method(D_METHOD("set_height_curve", "curve"), &ProceduralTerrainParameters::set_height_curve);
		ClassDB::bind_method(D_METHOD("get_height_curve"), &ProceduralTerrainParameters::get_height_curve);

        ClassDB::bind_method(D_METHOD("set_color_map", "color_map"), &ProceduralTerrainParameters::set_color_map);
        ClassDB::bind_method(D_METHOD("get_color_map"), &ProceduralTerrainParameters::get_color_map);
        
        ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &ProceduralTerrainParameters::set_octaves);
		ClassDB::bind_method(D_METHOD("get_octaves"), &ProceduralTerrainParameters::get_octaves);

		ClassDB::bind_method(D_METHOD("set_lacunarity", "lacunarity"), &ProceduralTerrainParameters::set_lacunarity);
		ClassDB::bind_method(D_METHOD("get_lacunarity"), &ProceduralTerrainParameters::get_lacunarity);

		ClassDB::bind_method(D_METHOD("set_persistence", "persistence"), &ProceduralTerrainParameters::set_persistence);
		ClassDB::bind_method(D_METHOD("get_persistence"), &ProceduralTerrainParameters::get_persistence);
		
		ClassDB::bind_method(D_METHOD("set_height_scale", "scale"), &ProceduralTerrainParameters::set_height_scale);
		ClassDB::bind_method(D_METHOD("get_height_scale"), &ProceduralTerrainParameters::get_height_scale);
        
		ClassDB::bind_method(D_METHOD("set_falloff", "falloff"), &ProceduralTerrainParameters::set_falloff);
		ClassDB::bind_method(D_METHOD("get_falloff"), &ProceduralTerrainParameters::get_falloff);

        ClassDB::bind_method(D_METHOD("set_flatshaded", "flatshaded"), &ProceduralTerrainParameters::set_flatshaded);
        ClassDB::bind_method(D_METHOD("get_flatshaded"), &ProceduralTerrainParameters::get_flatshaded);

        ADD_PROPERTY(PropertyInfo(Variant::INT, "generation_mode", PROPERTY_HINT_ENUM, "Normal,Falloff,Noise Unshaded,Noise Shaded"), "set_generation_mode", "get_generation_mode");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_height_curve", "get_height_curve");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "color_map", PROPERTY_HINT_RESOURCE_TYPE, "Gradient"), "set_color_map", "get_color_map");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves", PROPERTY_HINT_RANGE, itos(MIN_OCTAVES) + "," + itos(MAX_OCTAVES) + ",1"), "set_octaves", "get_octaves");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lacunarity"), "set_lacunarity", "get_lacunarity");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "persistence"), "set_persistence", "get_persistence");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale"), "set_height_scale", "get_height_scale");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "falloff"), "set_falloff", "get_falloff");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flatshaded"), "set_flatshaded", "get_flatshaded");

        BIND_CONSTANT(MIN_LEVEL_OF_DETAIL);
        BIND_CONSTANT(MAX_LEVEL_OF_DETAIL);
        BIND_CONSTANT(MIN_OCTAVES);
        BIND_CONSTANT(MAX_OCTAVES);

        BIND_ENUM_CONSTANT(GENERATION_MODE_NORMAL);
        BIND_ENUM_CONSTANT(GENERATION_MODE_FALLOFF);
        BIND_ENUM_CONSTANT(GENERATION_MODE_NOISE_UNSHADED);
        BIND_ENUM_CONSTANT(GENERATION_MODE_NOISE_SHADED);
    };

private:
    void update() { emit_changed(); }
};

VARIANT_ENUM_CAST(ProceduralTerrainParameters::GenerationMode);

#endif
