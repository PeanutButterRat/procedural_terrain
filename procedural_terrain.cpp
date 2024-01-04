#include "procedural_terrain.h"


#include "scene/resources/curve.h"
#include "scene/resources/texture.h"
#include "scene/resources/surface_tool.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/image_texture.h"

#include "core/math/random_number_generator.h"
#include "core/math/math_funcs.h"
#include "core/typedefs.h"
#include "core/core_bind.h"

#include "modules/noise/fastnoise_lite.h"

#include "chunk.h"

#define RESET_CHUNKS_ON_CHANGE if (reset_chunks_on_change) { reset_chunks(); }

constexpr int matrix_size = 241;
constexpr real_t half_matrix_size = matrix_size / 2.0f;
constexpr real_t max_offset = 100'000.0f;
constexpr real_t center_offset = (matrix_size - 1) / 2.0f;
constexpr int min_octaves = 1;
constexpr int max_octaves = 10;
constexpr int min_level_of_detail = 0;
constexpr int max_level_of_detail = 6;
constexpr int chunk_size = matrix_size - 1;
constexpr real_t normalization_factor = 0.9f;


void ProceduralTerrain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &ProceduralTerrain::set_octaves);
	ClassDB::bind_method(D_METHOD("get_octaves"), &ProceduralTerrain::get_octaves);

	ClassDB::bind_method(D_METHOD("set_lacunarity", "lacunarity"), &ProceduralTerrain::set_lacunarity);
	ClassDB::bind_method(D_METHOD("get_lacunarity"), &ProceduralTerrain::get_lacunarity);

	ClassDB::bind_method(D_METHOD("set_persistence", "persistence"), &ProceduralTerrain::set_persistence);
	ClassDB::bind_method(D_METHOD("get_persistence"), &ProceduralTerrain::get_persistence);
	
	ClassDB::bind_method(D_METHOD("set_height_scale", "scale"), &ProceduralTerrain::set_height_scale);
	ClassDB::bind_method(D_METHOD("get_height_scale"), &ProceduralTerrain::get_height_scale);

	ClassDB::bind_method(D_METHOD("set_noise", "noise"), &ProceduralTerrain::set_noise);
	ClassDB::bind_method(D_METHOD("get_noise"), &ProceduralTerrain::get_noise);

	ClassDB::bind_method(D_METHOD("set_height_curve", "curve"), &ProceduralTerrain::set_height_curve);
	ClassDB::bind_method(D_METHOD("get_height_curve"), &ProceduralTerrain::get_height_curve);

	ClassDB::bind_method(D_METHOD("set_observer", "observer"), &ProceduralTerrain::set_observer);
	ClassDB::bind_method(D_METHOD("get_observer"), &ProceduralTerrain::get_observer);

	ClassDB::bind_method(D_METHOD("set_view_distance", "view_distance"), &ProceduralTerrain::set_view_distance);
	ClassDB::bind_method(D_METHOD("get_view_distance"), &ProceduralTerrain::get_view_distance);

	ClassDB::bind_method(D_METHOD("set_view_thresholds", "view_thresholds"), &ProceduralTerrain::set_view_thresholds);
	ClassDB::bind_method(D_METHOD("get_view_thresholds"), &ProceduralTerrain::get_view_thresholds);

	ClassDB::bind_method(D_METHOD("set_reset_chunks_on_change", "value"), &ProceduralTerrain::set_reset_chunks_on_change);
	ClassDB::bind_method(D_METHOD("get_reset_chunks_on_change"), &ProceduralTerrain::get_reset_chunks_on_change);

	ClassDB::bind_method(D_METHOD("set_falloff_parameters", "parameters"), &ProceduralTerrain::set_falloff_parameters);
	ClassDB::bind_method(D_METHOD("get_falloff_parameters"), &ProceduralTerrain::get_falloff_parameters);
	
	ClassDB::bind_method(D_METHOD("reset_chunks"), &ProceduralTerrain::reset_chunks);
	
	ClassDB::bind_static_method("ProceduralTerrain",
		D_METHOD("generate_chunk", "noise", "height_curve", "level_of_detail", "material", "octaves", "persistence", "lacunarity", "height_scale", "offset", "falloff_parameters"),
		&generate_chunk, DEFVAL(Ref<StandardMaterial3D>{}), DEFVAL(min_octaves), DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(Vector2{}), DEFVAL(Vector2{}));
	
	BIND_CONSTANT(min_level_of_detail);
	BIND_CONSTANT(max_level_of_detail);
	BIND_CONSTANT(min_octaves);
	BIND_CONSTANT(max_octaves);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reset_chunks_on_change"), "set_reset_chunks_on_change", "get_reset_chunks_on_change");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "observer", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_observer", "get_observer");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "view_distance"), "set_view_distance", "get_view_distance");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "view_thresholds"), "set_view_thresholds", "get_view_thresholds");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves", PROPERTY_HINT_RANGE, itos(min_octaves) + "," + itos(max_octaves) + ",1"), "set_octaves", "get_octaves");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lacunarity"), "set_lacunarity", "get_lacunarity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "persistence"), "set_persistence", "get_persistence");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_height_curve", "get_height_curve");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "falloff_parameters"), "set_falloff_parameters", "get_falloff_parameters");
}

void ProceduralTerrain::_notification(const int p_what) {
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		_update();
	}
}

void ProceduralTerrain::set_octaves(const int p_octaves) {
	RESET_CHUNKS_ON_CHANGE
	octaves = CLAMP(p_octaves, min_octaves, max_octaves);
}

int ProceduralTerrain::get_octaves() const {
	return octaves;
}

void ProceduralTerrain::set_lacunarity(const real_t p_lacunarity) {
	RESET_CHUNKS_ON_CHANGE
	lacunarity = p_lacunarity;
}

real_t ProceduralTerrain::get_lacunarity() const {
	return lacunarity;
}

void ProceduralTerrain::set_persistence(const real_t p_persistence) {
	RESET_CHUNKS_ON_CHANGE
	persistence = p_persistence;
}

real_t ProceduralTerrain::get_persistence() const {
	return persistence;
}

void ProceduralTerrain::set_height_scale(const real_t p_height_scale) {
	RESET_CHUNKS_ON_CHANGE
	height_scale = p_height_scale;
}

real_t ProceduralTerrain::get_height_scale() const {
	return height_scale;
}

void ProceduralTerrain::set_noise(const Ref<FastNoiseLite>& p_noise) {
	RESET_CHUNKS_ON_CHANGE
	noise = p_noise;
}

Ref<FastNoiseLite> ProceduralTerrain::get_noise() const {
	return noise;
}

void ProceduralTerrain::set_height_curve(const Ref<Curve>& p_height_curve) {
	RESET_CHUNKS_ON_CHANGE
	height_curve = p_height_curve;
}

Ref<Curve> ProceduralTerrain::get_height_curve() const {
	return height_curve;
}

void ProceduralTerrain::set_observer(const NodePath& p_observer) {
	RESET_CHUNKS_ON_CHANGE
	observer = p_observer;
}

NodePath ProceduralTerrain::get_observer() const {
	return observer;
}

void ProceduralTerrain::set_view_distance(const real_t p_view_distance) {
	view_distance = p_view_distance;
}

real_t ProceduralTerrain::get_view_distance() const {
	return view_distance;
}

void ProceduralTerrain::set_view_thresholds(PackedFloat32Array p_view_thresholds) {
	p_view_thresholds.resize(max_level_of_detail + 1);
	p_view_thresholds.sort();
	p_view_thresholds.reverse();
	view_thresholds = p_view_thresholds;
}

PackedFloat32Array ProceduralTerrain::get_view_thresholds() const {
	return view_thresholds;
}

void ProceduralTerrain::set_reset_chunks_on_change(bool p_reset_chunks_on_change) {
	reset_chunks_on_change = p_reset_chunks_on_change;
}

bool ProceduralTerrain::get_reset_chunks_on_change() const {
	return reset_chunks_on_change;
}

void ProceduralTerrain::set_falloff_parameters(Vector2 parameters) {
	RESET_CHUNKS_ON_CHANGE
	falloff_parameters = parameters;
}

Vector2 ProceduralTerrain::get_falloff_parameters() const {
	return falloff_parameters;
}

void ProceduralTerrain::reset_chunks() {
	const Array chunks = generated_chunks.values();
	for (int i = 0; i < chunks.size(); i++) {
		Chunk* chunk = cast_to<Chunk>(chunks[i]);
		chunk->queue_free();
	}
	generated_chunks.clear();
	visible_chunks.clear();
}

void ProceduralTerrain::_update() {
	for (int i = 0; i < visible_chunks.size(); i++) {
		Chunk* chunk = cast_to<Chunk>(visible_chunks[i]);
		chunk->set_visible(false);
	}
	visible_chunks.clear();
	
	const Node3D* observer_node = has_node(observer) ? cast_to<Node3D>(get_node(observer)) : nullptr;
	
	if (observer_node != nullptr) {
		const Vector2 relative_observer_position {
			(observer_node->get_global_position() - get_global_position()).x,
			(observer_node->get_global_position() - get_global_position()).z
		};
		const real_t x = round(relative_observer_position.x / (chunk_size * get_scale().x));
		const real_t y = round(relative_observer_position.y / (chunk_size * get_scale().z));
		const int chunks_in_view_distance_x = round(view_distance / (chunk_size * get_scale().x));
		const int chunks_in_view_distance_y = round(view_distance / (chunk_size * get_scale().z));
		
		for (int y_offset = -chunks_in_view_distance_y; y_offset <= chunks_in_view_distance_y; y_offset++) {
			for (int x_offset = -chunks_in_view_distance_x; x_offset <= chunks_in_view_distance_x; x_offset++) {
				const Vector2 chunk_coordinates = Vector2(x + x_offset, y + y_offset);
				if (!generated_chunks.has(chunk_coordinates)) {
					Chunk* chunk = memnew(Chunk);
					generated_chunks[chunk_coordinates] = chunk;
					chunk->set_name("__Chunk " + chunk_coordinates);
					add_child(chunk, false, INTERNAL_MODE_BACK);
					
					const Vector2 chunk_position = chunk_coordinates * chunk_size;
					chunk->set_position(Vector3(chunk_position.x, 0, chunk_position.y));
				}
				
				Chunk* chunk = cast_to<Chunk>(generated_chunks[chunk_coordinates]);
				const float distance_to_chunk = _get_distance_to_chunk(chunk);
				chunk->set_visible(distance_to_chunk <= view_distance);

				if (chunk->is_visible()) {
					int level_of_detail = 0;
					while (level_of_detail < view_thresholds.size() - 1 && distance_to_chunk <= view_thresholds[level_of_detail + 1]) {
						level_of_detail++;
					}

					if (!noise.is_null() && !height_curve.is_null()) {
						chunk->request_mesh(level_of_detail);
					}
					visible_chunks.append(chunk);
				}
			}
		}
	}
}

Ref<Mesh> ProceduralTerrain::generate_chunk(const Ref<FastNoiseLite>& noise, const Ref<Curve>& height_curve, const int level_of_detail,
	const Ref<StandardMaterial3D>& material, const int octaves, const real_t persistence, const real_t lacunarity, const real_t height_scale,
	Vector2 offset, Vector2 falloff_parameters) {

	Array matrix = _generate_matrix(octaves, noise, persistence, lacunarity, offset);
	const Ref<ArrayMesh> mesh = _generate_mesh(matrix, level_of_detail, height_curve, height_scale);

	if (falloff_parameters != Vector2{}) {
		Array map = _generate_falloff(falloff_parameters);
		int index = 0;
		for (int y = 0; y < matrix_size; y++) {
			for (int x = 0; x < matrix_size; x++) {
				const real_t falloff = map[index];
				const real_t previous_value = matrix[index];
				matrix[index] = previous_value - falloff;
				index++;
			}
		}
	}
	
	if (!material.is_null()) {
		_generate_material(matrix, material);
	}
	
	
	return mesh;
}

float ProceduralTerrain::_get_distance_to_chunk(const Chunk* chunk) const {
	const Vector3 start = chunk->get_global_position();
	Vector3 end = chunk->get_global_position();
	end.x += chunk->get_aabb().size.x * get_scale().x;
	end.z += chunk->get_aabb().size.z * get_scale().z;
	const Vector3 point_of_reference = dynamic_cast<Node3D*>(get_node(observer))->get_global_position();
	real_t dx = MAX(start.x - point_of_reference.x, point_of_reference.x - end.x);
	real_t dy = MAX(start.z - point_of_reference.z, point_of_reference.z - end.z);
	dx = MAX(dx, 0);
	dy = MAX(dy, 0);
	
	return sqrt(dx * dx + dy * dy);
}

ProceduralTerrain::ProceduralTerrain() {
	octaves = 1;
	lacunarity = 1.0f;
	persistence = 1.0f;
	height_scale = 1.0f;
	view_distance = 300.0f;
	reset_chunks_on_change = true;
	const PackedFloat32Array thresholds = {100, 150, 200, 250, 300, 350, 400};
	set_view_thresholds(thresholds);
	set_process_internal(true);
}

Array ProceduralTerrain::_generate_matrix(const int octaves, const Ref<FastNoiseLite>& noise, const real_t persistence, const real_t lacunarity, Vector2 offset) {
	Array matrix{};
	matrix.resize(matrix_size * matrix_size);
	Array offsets{};
	offsets.resize(octaves);
	RandomNumberGenerator rng{};
	rng.set_seed(noise->get_seed());

	real_t max_possible_height = 0.0f;
	real_t max_possible_amplitude = 1.0f;
	
	for (int i = 0; i < octaves; i++) {
		const real_t x = rng.randf_range(-max_offset, max_offset) + offset.x;
		const real_t y = rng.randf_range(-max_offset, max_offset) + offset.y;
		offsets[i] = Vector2(x, y);

		max_possible_height += max_possible_amplitude;
		max_possible_amplitude *= persistence;
	}
	
	int index = 0;

	for (int y = 0; y < matrix_size; y++) {
		for (int x = 0; x < matrix_size; x++) {
			real_t amplitude = 1.0f;
			real_t frequency = 1.0f;
			real_t final_value = 0.0f;
			
			for (int octave = 0; octave < octaves; octave++) {
				const Vector2 offset = offsets[octave];
				const real_t sample_x = (x - half_matrix_size + offset.x) * frequency;
				const real_t sample_y = (y - half_matrix_size + offset.y) * frequency;
				const real_t sample = noise->get_noise_2d(sample_x, sample_y);
				final_value += sample * amplitude;
				amplitude *= persistence;
				frequency *= lacunarity;
			}

			matrix[y * matrix_size + x] = final_value;
			index++;
		}
	}
	
	index = 0;

	for (int y = 0; y < matrix_size; y++) {
		for (int x = 0; x < matrix_size; x++) {
			const real_t bounds = max_possible_height * normalization_factor;
			const real_t normalized_height = Math::inverse_lerp(-bounds, bounds, matrix[index]);
			matrix[index] = normalized_height;
			index++;
		}
	}
	
	return matrix;
}

Ref<ArrayMesh> ProceduralTerrain::_generate_mesh(const Array& matrix, const int level_of_detail, const Ref<Curve>& height_curve, const real_t height_scale) {
	const int increment = CLAMP((max_level_of_detail - level_of_detail) * 2, 1, 12);
	const int vertices_per_line = (matrix_size - 1) / increment + 1;

	Array arrays{};
	arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertices{};
	vertices.resize(pow(vertices_per_line, 2));

	PackedVector2Array uvs{};
	uvs.resize(vertices.size());

	PackedInt32Array indices{};
	indices.resize(pow(vertices_per_line - 1, 2) * 6);

	PackedVector3Array normals{};
	normals.resize(vertices.size());
	
	int vertex_index = 0;
	int indices_index = 0;
	
	for (int y = 0; y < matrix_size; y += increment) {
		for (int x = 0; x < matrix_size; x += increment) {
			const real_t height = height_curve->sample(matrix[y * matrix_size + x]) * height_scale;
			vertices.set(vertex_index, Vector3(x - center_offset, height, y - center_offset));
			uvs.set(vertex_index, Vector2(static_cast<real_t>(x) / matrix_size, static_cast<real_t>(y) / matrix_size));
			
			if (x < matrix_size - 1 && y < matrix_size - 1) {
				indices.set(indices_index++, vertex_index);
				indices.set(indices_index++, vertex_index + vertices_per_line + 1);
				indices.set(indices_index++, vertex_index + vertices_per_line);

				indices.set(indices_index++, vertex_index + vertices_per_line + 1);
				indices.set(indices_index++, vertex_index);
				indices.set(indices_index++, vertex_index + 1);
			}
			
			vertex_index++;
		}
	}

	const int number_of_triangles = indices.size() / 3;

	for (int i = 0; i < number_of_triangles; i++) {
		const int triangle_index = i * 3;
		const int a_index = indices[triangle_index];
		const int b_index = indices[triangle_index + 1];
		const int c_index = indices[triangle_index + 2];
		
		Vector3 a = vertices[a_index];
		Vector3 b = vertices[b_index];
		Vector3 c = vertices[c_index];
		Vector3 normal = Plane(a, b, c).normal;
		
		normals.set(a_index, normals[a_index] + normal);
		normals.set(b_index, normals[b_index] + normal);
		normals.set(c_index, normals[c_index] + normal);
	}

	for (int i = 0; i < normals.size(); i++) {
		normals.set(i, normals[i].normalized());
	}
	
	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	return mesh;
}

void ProceduralTerrain::_generate_material(const Array& matrix, const Ref<StandardMaterial3D>& material) {
	const Ref<Image> image = Image::create_empty(matrix_size, matrix_size, false, Image::FORMAT_RGB8);
	for (int y = 0; y < matrix_size; y++) {
		for (int x = 0; x < matrix_size; x++) {
			const real_t height = matrix[y * matrix_size + x];
			Color color;
			
			if (height < 0.3f) {
				color = Color(21.0f/255, 106.0f/255, 179.0f/255);
			} else if (height < 0.4f) {
				color = Color(72.0f/255, 151.0f/255, 219.0f/255);
			} else if (height < 0.45f) {
				color = Color(235.0f/255, 228.0f/255, 103.0f/255);
			} else if (height < 0.55f) {
				color = Color(46.0f/255, 148.0f/255, 51.0f/255);
			} else if (height < 0.6f) {
				color = Color(35.0f/255, 105.0f/255, 38.0f/255);
			} else if (height < 0.7f) {
				color = Color(51.0f/255, 41.0f/255, 37.0f/255);
			} else if (height < 0.9f) {
				color = Color(31.0f/255, 25.0f/255, 22.0f/255);
			} else {
				color = Color(1.0f, 1.0f, 1.0f);
			}
			
			image->set_pixel(x, y, color);
		}
	}
	
	const Ref<Texture> texture = ImageTexture::create_from_image(image);
	
	material->set_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO, texture);
}

Array ProceduralTerrain::_generate_falloff(Vector2 falloff_parameters) {
	Array map{};
	map.resize(matrix_size * matrix_size);
	int index = 0;
	
	for (int i = 0; i < matrix_size; i++) {
		for (int j = 0; j < matrix_size; j++) {
			const real_t x = static_cast<real_t>(i) / matrix_size * 2 - 1;
			const real_t y = static_cast<real_t>(j) / matrix_size * 2 - 1;
			const real_t value = MAX(ABS(x), ABS(y));

			const real_t a = falloff_parameters.x;
			const real_t b = falloff_parameters.y;
			
			map[index] = pow(value, a) / (pow(value, a) + pow(b - b * value, a));
			index++;
		}
	}
	
	return map;
}
