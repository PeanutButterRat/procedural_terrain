#include "procedural_terrain.h"


#include "scene/resources/curve.h"
#include "scene/resources/texture.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/image_texture.h"

#include "core/math/random_number_generator.h"
#include "core/math/math_funcs.h"
#include "core/typedefs.h"
#include "core/core_bind.h"

#include "procedural_terrain_parameters.h"
#include "procedural_terrain_chunk.h"

constexpr int MATRIX_SIZE = 241;
constexpr int CHUNK_SIZE = MATRIX_SIZE - 1;
constexpr real_t HALF_MATRIX_SIZE = MATRIX_SIZE / 2.0f;
constexpr real_t MAX_OFFSET = 100'000.0f;
constexpr real_t CENTER_OFFSET = (MATRIX_SIZE - 1) / 2.0f;
constexpr real_t NORMALIZATION_FACTOR = 0.9f;


void ProceduralTerrain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_viewer", "observer"), &ProceduralTerrain::set_viewer);
	ClassDB::bind_method(D_METHOD("get_viewer"), &ProceduralTerrain::get_viewer);
	
	ClassDB::bind_method(D_METHOD("set_view_thresholds", "view_thresholds"), &ProceduralTerrain::set_view_thresholds);
	ClassDB::bind_method(D_METHOD("get_view_thresholds"), &ProceduralTerrain::get_view_thresholds);
	
	ClassDB::bind_method(D_METHOD("set_terrain_parameters", "parameters"), &ProceduralTerrain::set_terrain_parameters);
	ClassDB::bind_method(D_METHOD("get_terrain_parameters"), &ProceduralTerrain::get_terrain_parameters);
	
	ClassDB::bind_method(D_METHOD("clear_chunks"), &ProceduralTerrain::clear_chunks);
	ClassDB::bind_static_method("ProceduralTerrain", D_METHOD("generate_terrain", "parameters", "material"), &generate_terrain, DEFVAL(Ref<StandardMaterial3D>{}));
	
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "viewer", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_viewer", "get_viewer");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "view_thresholds"), "set_view_thresholds", "get_view_thresholds");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parameters", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralTerrainParameters"), "set_terrain_parameters", "get_terrain_parameters");
}

ProceduralTerrain::ProceduralTerrain() {
	const PackedFloat32Array thresholds = {100, 150, 200, 250, 300, 350, 400};
	set_view_thresholds(thresholds);
	set_process_internal(true);
}

void ProceduralTerrain::_notification(const int p_what) {
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		_update();
	}
}

void ProceduralTerrain::set_view_thresholds(PackedFloat32Array p_view_thresholds) {
	p_view_thresholds.resize(MAX_LEVEL_OF_DETAIL + 1);
	p_view_thresholds.sort();
	p_view_thresholds.reverse();
	view_thresholds = p_view_thresholds;
}

void ProceduralTerrain::clear_chunks() {
	const Array chunks = generated_chunks.values();
	for (int i = 0; i < chunks.size(); i++) {
		ProceduralTerrainChunk* chunk = cast_to<ProceduralTerrainChunk>(chunks[i]);
		chunk->queue_free();
	}
	generated_chunks.clear();
	visible_chunks.clear();
}

void ProceduralTerrain::_update() {
	for (int i = 0; i < visible_chunks.size(); i++) {
		ProceduralTerrainChunk* chunk = cast_to<ProceduralTerrainChunk>(visible_chunks[i]);
		chunk->set_visible(false);
	}
	visible_chunks.clear();
	
	const Node3D* observer_node = has_node(viewer) ? cast_to<Node3D>(get_node(viewer)) : nullptr;
	const real_t view_distance = view_thresholds[0];
	if (observer_node != nullptr) {
		const Vector2 relative_observer_position {
			(observer_node->get_global_position() - get_global_position()).x,
			(observer_node->get_global_position() - get_global_position()).z
		};
		const real_t x = round(relative_observer_position.x / (CHUNK_SIZE * get_scale().x));
		const real_t y = round(relative_observer_position.y / (CHUNK_SIZE * get_scale().z));
		const int chunks_in_view_distance_x = round(view_distance / (CHUNK_SIZE * get_scale().x));
		const int chunks_in_view_distance_y = round(view_distance / (CHUNK_SIZE * get_scale().z));
		
		for (int y_offset = -chunks_in_view_distance_y; y_offset <= chunks_in_view_distance_y; y_offset++) {
			for (int x_offset = -chunks_in_view_distance_x; x_offset <= chunks_in_view_distance_x; x_offset++) {
				const Vector2 chunk_coordinates = Vector2(x + x_offset, y + y_offset);
				if (!generated_chunks.has(chunk_coordinates)) {
					ProceduralTerrainChunk* chunk = memnew(ProceduralTerrainChunk);
					generated_chunks[chunk_coordinates] = chunk;
					chunk->set_name("__Chunk " + chunk_coordinates);
					add_child(chunk, false, INTERNAL_MODE_BACK);
					
					const Vector2 chunk_position = chunk_coordinates * CHUNK_SIZE;
					chunk->set_position(Vector3(chunk_position.x, 0, chunk_position.y));
				}
				
				ProceduralTerrainChunk* chunk = cast_to<ProceduralTerrainChunk>(generated_chunks[chunk_coordinates]);
				const float distance_to_chunk = _get_distance_to_chunk(chunk);

				int level_of_detail = -1;
				while (level_of_detail < view_thresholds.size() - 1 && distance_to_chunk <= view_thresholds[level_of_detail + 1]) {
					level_of_detail++;
				}

				if (level_of_detail >= 0) {
					chunk->request_mesh(level_of_detail);
					chunk->set_visible(true);
					visible_chunks.append(chunk);
				}
			}
		}
	}
}

Ref<Mesh> ProceduralTerrain::generate_terrain(const Ref<ProceduralTerrainParameters>& parameters, const Ref<StandardMaterial3D>& material) {
	Array matrix = _generate_matrix(parameters->get_octaves(), parameters->get_noise(), parameters->get_persistence(), parameters->get_lacunarity());
	const Ref<ArrayMesh> mesh = _generate_mesh(matrix, parameters->get_level_of_detail(), parameters->get_height_curve(), parameters->get_height_scale());
	
	if (parameters->get_falloff() != Vector2{}) {
		Array map = _generate_falloff(parameters->get_falloff());
		int index = 0;
		for (int y = 0; y < MATRIX_SIZE; y++) {
			for (int x = 0; x < MATRIX_SIZE; x++) {
				const real_t falloff_amount = map[index];
				const real_t previous_value = matrix[index];
				matrix[index] = previous_value - falloff_amount;
				index++;
			}
		}
	}
	
	if (material.is_valid()) {
		_generate_material(matrix, material);
	}
	
	return mesh;
}

float ProceduralTerrain::_get_distance_to_chunk(const ProceduralTerrainChunk* chunk) const {
	const Vector3 start = chunk->get_global_position();
	Vector3 end = chunk->get_global_position();
	end.x += chunk->get_aabb().size.x * get_scale().x;
	end.z += chunk->get_aabb().size.z * get_scale().z;
	const Vector3 point_of_reference = dynamic_cast<Node3D*>(get_node(viewer))->get_global_position();
	real_t dx = MAX(start.x - point_of_reference.x, point_of_reference.x - end.x);
	real_t dy = MAX(start.z - point_of_reference.z, point_of_reference.z - end.z);
	dx = MAX(dx, 0);
	dy = MAX(dy, 0);
	
	return sqrt(dx * dx + dy * dy);
}

Array ProceduralTerrain::_generate_matrix(const int octaves, const Ref<FastNoiseLite>& noise, const real_t persistence, const real_t lacunarity) {
	Array matrix{};
	matrix.resize(MATRIX_SIZE * MATRIX_SIZE);
	Array offsets{};
	offsets.resize(octaves);
	RandomNumberGenerator rng{};
	rng.set_seed(noise->get_seed());

	real_t max_possible_height = 0.0f;
	real_t max_possible_amplitude = 1.0f;
	
	for (int i = 0; i < octaves; i++) {
		const real_t x = rng.randf_range(-MAX_OFFSET, MAX_OFFSET);
		const real_t y = rng.randf_range(-MAX_OFFSET, MAX_OFFSET);
		offsets[i] = Vector2(x, y);

		max_possible_height += max_possible_amplitude;
		max_possible_amplitude *= persistence;
	}
	
	int index = 0;

	for (int y = 0; y < MATRIX_SIZE; y++) {
		for (int x = 0; x < MATRIX_SIZE; x++) {
			real_t amplitude = 1.0f;
			real_t frequency = 1.0f;
			real_t final_value = 0.0f;
			
			for (int octave = 0; octave < octaves; octave++) {
				const Vector2 offset = offsets[octave];
				const real_t sample_x = (-x - HALF_MATRIX_SIZE + offset.x) * frequency;
				const real_t sample_y = (y - HALF_MATRIX_SIZE + offset.y) * frequency;
				const real_t sample = noise->get_noise_3d(sample_y, sample_x, 0.0f);
				
				final_value += sample * amplitude;
				amplitude *= persistence;
				frequency *= lacunarity;
			}

			matrix[y * MATRIX_SIZE + x] = final_value;
			index++;
		}
	}
	
	index = 0;

	for (int y = 0; y < MATRIX_SIZE; y++) {
		for (int x = 0; x < MATRIX_SIZE; x++) {
			const real_t bounds = max_possible_height * NORMALIZATION_FACTOR;
			const real_t normalized_height = Math::inverse_lerp(-bounds, bounds, matrix[index]);
			matrix[index] = normalized_height;
			index++;
		}
	}
	
	return matrix;
}

Ref<ArrayMesh> ProceduralTerrain::_generate_mesh(const Array& matrix, const int level_of_detail, const Ref<Curve>& height_curve, const real_t height_scale) {
	const int increment = CLAMP((MAX_LEVEL_OF_DETAIL - level_of_detail) * 2, 1, 12);
	const int vertices_per_line = (MATRIX_SIZE - 1) / increment + 1;

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
	
	for (int y = 0; y < MATRIX_SIZE; y += increment) {
		for (int x = 0; x < MATRIX_SIZE; x += increment) {
			const real_t height = height_curve->sample(matrix[y * MATRIX_SIZE + x]) * height_scale;
			vertices.set(vertex_index, Vector3(x - CENTER_OFFSET, height, y - CENTER_OFFSET));
			uvs.set(vertex_index, Vector2(static_cast<real_t>(x) / MATRIX_SIZE, static_cast<real_t>(y) / MATRIX_SIZE));
			
			if (x < MATRIX_SIZE - 1 && y < MATRIX_SIZE - 1) {
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
	const Ref<Image> image = Image::create_empty(MATRIX_SIZE, MATRIX_SIZE, false, Image::FORMAT_RGB8);
	for (int y = 0; y < MATRIX_SIZE; y++) {
		for (int x = 0; x < MATRIX_SIZE; x++) {
			const real_t height = matrix[y * MATRIX_SIZE + x];
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

Array ProceduralTerrain::_generate_falloff(const Vector2 falloff) {
	Array map{};
	map.resize(MATRIX_SIZE * MATRIX_SIZE);
	int index = 0;
	
	for (int i = 0; i < MATRIX_SIZE; i++) {
		for (int j = 0; j < MATRIX_SIZE; j++) {
			const real_t x = static_cast<real_t>(i) / MATRIX_SIZE * 2 - 1;
			const real_t y = static_cast<real_t>(j) / MATRIX_SIZE * 2 - 1;
			const real_t value = MAX(ABS(x), ABS(y));

			const real_t a = falloff.x;
			const real_t b = falloff.y;
			
			map[index] = pow(value, a) / (pow(value, a) + pow(b - b * value, a));
			index++;
		}
	}
	
	return map;
}
