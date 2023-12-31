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

#include <limits>

constexpr int matrix_size = 241;
constexpr real_t half_matrix_size = matrix_size / 2.0f;
constexpr real_t max_offset = 100'000.0f;
constexpr real_t center_offset = (matrix_size - 1) / 2.0f;
constexpr real_t maximum_real_t_magnitude = std::numeric_limits<real_t>::max();
constexpr int min_octaves = 1;
constexpr int max_octaves = 10;
constexpr int min_level_of_detail = 0;
constexpr int max_level_of_detail = 6;
constexpr int chunk_size = matrix_size - 1;


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

	ClassDB::bind_static_method("ProceduralTerrain",
		D_METHOD("generate_chunk", "noise", "height_curve", "level_of_detail", "material", "octaves", "persistence", "lacunarity", "height_scale"),
		&generate_chunk, DEFVAL(Ref<StandardMaterial3D>{}), DEFVAL(min_octaves), DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(1.0f));
	
	BIND_CONSTANT(min_level_of_detail);
	BIND_CONSTANT(max_level_of_detail);
	BIND_CONSTANT(min_octaves);
	BIND_CONSTANT(max_octaves);
	
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "observer", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_observer", "get_observer");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "view_distance"), "set_view_distance", "get_view_distance");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "view_thresholds"), "set_view_thresholds", "get_view_thresholds");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves", PROPERTY_HINT_RANGE, itos(min_octaves) + "," + itos(max_octaves) + ",1"), "set_octaves", "get_octaves");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lacunarity"), "set_lacunarity", "get_lacunarity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "persistence"), "set_persistence", "get_persistence");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale"), "set_height_scale", "get_height_scale");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_height_curve", "get_height_curve");
}

void ProceduralTerrain::_notification(const int p_what) {
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		_update();
	}
}

void Chunk::request_mesh(int level_of_detail) {
	level_of_detail = CLAMP(level_of_detail, min_level_of_detail, max_level_of_detail);
	
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
			terrain->get("height_scale")
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

void ProceduralTerrain::set_octaves(const int p_octaves) {
	octaves = CLAMP(p_octaves, min_octaves, max_octaves);
}

int ProceduralTerrain::get_octaves() const {
	return octaves;
}

void ProceduralTerrain::set_lacunarity(const real_t p_lacunarity) {
	lacunarity = p_lacunarity;
}

real_t ProceduralTerrain::get_lacunarity() const {
	return lacunarity;
}

void ProceduralTerrain::set_persistence(const real_t p_persistence) {
	persistence = p_persistence;
}

real_t ProceduralTerrain::get_persistence() const {
	return persistence;
}

void ProceduralTerrain::set_height_scale(const real_t p_height_scale) {
	height_scale = p_height_scale;
}

real_t ProceduralTerrain::get_height_scale() const {
	return height_scale;
}

void ProceduralTerrain::set_noise(const Ref<FastNoiseLite>& p_noise) {
	noise = p_noise;
}

Ref<FastNoiseLite> ProceduralTerrain::get_noise() const {
	return noise;
}

void ProceduralTerrain::set_height_curve(const Ref<Curve>& p_height_curve) {
	height_curve = p_height_curve;
}

Ref<Curve> ProceduralTerrain::get_height_curve() const {
	return height_curve;
}

void ProceduralTerrain::set_observer(const NodePath& p_observer) {
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

void ProceduralTerrain::_update() {
	for (int i = 0; i < visible_chunks.size(); i++) {
		Chunk* chunk = cast_to<Chunk>(visible_chunks[i]);
		chunk->set_visible(false);
	}
	visible_chunks.clear();
	
	const Node3D* observer_node = has_node(observer) ? cast_to<Node3D>(get_node(observer)) : nullptr;

	if (observer_node != nullptr) {
		const real_t x = round((observer_node->get_global_position() - get_global_position()).x / chunk_size);
		const real_t y = round((observer_node->get_global_position() - get_global_position()).z / chunk_size);
		const int chunks_in_view_distance = round(view_distance / chunk_size);

		for (int y_offset = -chunks_in_view_distance; y_offset <= chunks_in_view_distance; y_offset++) {
			for (int x_offset = -chunks_in_view_distance; x_offset <= chunks_in_view_distance; x_offset++) {
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
					chunk->request_mesh(level_of_detail);
					visible_chunks.append(chunk);
				}
			}
		}
	}
}

Ref<Mesh> ProceduralTerrain::generate_chunk(const Ref<FastNoiseLite>& noise, const Ref<Curve>& height_curve, const int level_of_detail,
	const Ref<StandardMaterial3D>& material, const int octaves, const real_t persistence, const real_t lacunarity, const real_t height_scale) {
	
	const Array matrix = _generate_matrix(octaves, noise, persistence, height_scale);
	const Ref<ArrayMesh> mesh = _generate_mesh(matrix, level_of_detail, height_curve, lacunarity);
	
	if (!material.is_null()) {
		_generate_material(matrix, material);
	}
	
	return mesh;
}

float ProceduralTerrain::_get_distance_to_chunk(const Chunk* chunk) const {
	const Vector3 start = chunk->get_position();
	const Vector3 end = chunk->get_position() + chunk->get_aabb().size;
	const Vector3 point_of_reference = dynamic_cast<Node3D*>(get_node(observer))->get_global_position() - get_global_position();
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
	PackedFloat32Array thresholds{};
	thresholds.resize(max_level_of_detail + 1);
	thresholds.fill(0.0f);
	set_view_thresholds(thresholds);
	set_process_internal(true);
}

Array ProceduralTerrain::_generate_matrix(const int octaves, const Ref<FastNoiseLite>& noise, const real_t persistence, const real_t lacunarity) {
	Array matrix{};
	matrix.resize(matrix_size * matrix_size);
	Array offsets{};
	offsets.resize(octaves);
	RandomNumberGenerator rng{};
	rng.set_seed(noise->get_seed());
	
	for (int i = 0; i < octaves; i++) {
		const real_t x = rng.randf_range(-max_offset, max_offset);
		const real_t y = rng.randf_range(-max_offset, max_offset);
		offsets[i] = Vector2(x, y);
	}
	
	int index = 0;
	real_t minimum = maximum_real_t_magnitude;
	real_t maximum = -maximum_real_t_magnitude;
	
	for (int y = 0; y < matrix_size; y++) {
		for (int x = 0; x < matrix_size; x++) {
			real_t amplitude = 1.0f;
			real_t frequency = 1.0f;
			real_t final_value = 0.0f;
			
			for (int octave = 0; octave < octaves; octave++) {
				const Vector2 offset = offsets[octave];
				const Vector2 sample_location {
					(x - half_matrix_size) * frequency + offset.x,
					(y - half_matrix_size) * frequency + offset.y,
				};
				
				const real_t sample = noise->get_noise_2d(sample_location.x, sample_location.y);
				final_value += sample * amplitude;
				amplitude *= persistence;
				frequency *= lacunarity;
			}

			matrix[y * matrix_size + x] = final_value;
			
			minimum = MIN(minimum, final_value);
			maximum = MAX(maximum, final_value);
			index++;
		}
	}
	
	index = 0;

	for (int y = 0; y < matrix_size; y++) {
		for (int x = 0; x < matrix_size; x++) {
			matrix[index] = Math::inverse_lerp(minimum, maximum, matrix[index]);
			index++;
		}
	}
	
	return matrix;
}

Ref<ArrayMesh> ProceduralTerrain::_generate_mesh(const Array& matrix, const int level_of_detail, const Ref<Curve>& height_curve, const real_t height_scale) {
	const int increment = CLAMP((max_level_of_detail - level_of_detail) * 2, 1, 12);
	const int vertices_per_line = (matrix_size - 1) / increment + 1;

	Array arrays = Array();
	arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertices = PackedVector3Array();
	vertices.resize(pow(vertices_per_line, 2));

	PackedVector2Array uvs = PackedVector2Array();
	uvs.resize(pow(vertices_per_line, 2));

	PackedInt32Array indices = PackedInt32Array();
	indices.resize(pow(vertices_per_line - 1, 2) * 6);
	
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
	
	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_VERTEX] = vertices;

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