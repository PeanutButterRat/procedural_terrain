#include "procedural_terrain.h"


#include "scene/resources/curve.h"
#include "scene/resources/texture.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/primitive_meshes.h"
#include "scene/3d/mesh_instance_3d.h"

#include "core/math/random_number_generator.h"
#include "core/math/math_funcs.h"
#include "core/typedefs.h"
#include "core/core_bind.h"

#include "procedural_terrain_parameters.h"

constexpr int MATRIX_SIZE = 241;
constexpr int CHUNK_SIZE = MATRIX_SIZE - 1;
constexpr real_t HALF_MATRIX_SIZE = MATRIX_SIZE / 2.0f;
constexpr real_t MAX_OFFSET = 100'000.0f;
constexpr real_t CENTER_OFFSET = (MATRIX_SIZE - 1) / 2.0f;
constexpr real_t NORMALIZATION_FACTOR = 0.9f;


void ProceduralTerrain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_viewer", "observer"), &ProceduralTerrain::set_viewer);
	ClassDB::bind_method(D_METHOD("get_viewer"), &ProceduralTerrain::get_viewer);
	
	ClassDB::bind_method(D_METHOD("set_detail_offsets", "detail_offsets"), &ProceduralTerrain::set_detail_offsets);
	ClassDB::bind_method(D_METHOD("get_detail_offsets"), &ProceduralTerrain::get_detail_offsets);
	
	ClassDB::bind_method(D_METHOD("set_terrain_parameters", "parameters"), &ProceduralTerrain::set_terrain_parameters);
	ClassDB::bind_method(D_METHOD("get_terrain_parameters"), &ProceduralTerrain::get_terrain_parameters);
	
	ClassDB::bind_method(D_METHOD("clear_chunks"), &ProceduralTerrain::clear_chunks);
	ClassDB::bind_static_method("ProceduralTerrain", D_METHOD("generate_terrain", "parameters"), &generate_terrain);
	
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "viewer", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_viewer", "get_viewer");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "detail_offsets"), "set_detail_offsets", "get_detail_offsets");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parameters", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralTerrainParameters"), "set_terrain_parameters", "get_terrain_parameters");
}

void ProceduralTerrain::clear_chunks() {
	Array values = threads.values();
	for (int i = 0; i < values.size(); i++) {
		const Ref<core_bind::Thread> thread = values[i];
		MeshInstance3D* chunk = cast_to<MeshInstance3D>(thread->wait_to_finish());
		chunk->queue_free();
	}

	values = generated_chunks.values();
	for (int i = 0; i < values.size(); i++) {
		MeshInstance3D* chunk = cast_to<MeshInstance3D>(values[i]);
		if (chunk) { chunk->queue_free(); }
	}
	
	generated_chunks.clear();
	visible_chunks.clear();
}

void ProceduralTerrain::_internal_process() {
	for (int i = 0; i < visible_chunks.size(); i++) {
		MeshInstance3D* chunk = cast_to<MeshInstance3D>(visible_chunks[i]);
		remove_child(chunk);
	}
	
	visible_chunks.clear();
	
	if (const Node3D* observer = cast_to<Node3D>(get_node_or_null(viewer))) {
		const Vector2 relative_observer_position {
			(observer->get_global_position() - get_global_position()).x,
			(observer->get_global_position() - get_global_position()).z
		};
		
		const real_t x = round(relative_observer_position.x / (CHUNK_SIZE * get_scale().x));
		const real_t y = round(relative_observer_position.y / (CHUNK_SIZE * get_scale().z));

		for (int ring = 0; ring < detail_offsets.size(); ring++) {
			for (int y_offset = -ring; y_offset <= ring; y_offset++) {
				for (int x_offset = -ring; x_offset <= ring; x_offset++) {
					if (ABS(y_offset) != ring && ABS(x_offset) != ring) {
						continue;
					}
					const int detail = detail_offsets[ring];
					const Vector3 chunk_coordinates{x + x_offset, y + y_offset, static_cast<real_t>(detail)};
					const Vector3 chunk_position = Vector3(chunk_coordinates.x, 0.0f, chunk_coordinates.y) * CHUNK_SIZE;

					if (threads.has(chunk_coordinates)) {
						const Ref<core_bind::Thread> thread = cast_to<core_bind::Thread>(threads[chunk_coordinates]);
						if (!thread->is_alive()) {
							MeshInstance3D* chunk = cast_to<MeshInstance3D>(thread->wait_to_finish());
							generated_chunks[chunk_coordinates] = chunk;
							threads.erase(chunk_coordinates);
							
							chunk->set_position(chunk_position);
						}
					} else if (!generated_chunks.has(chunk_coordinates)) {
						const Ref<core_bind::Thread> thread = memnew(core_bind::Thread);
						threads[chunk_coordinates] = thread;

						const Ref<ProceduralTerrainParameters> parameters = get_terrain_parameters()->duplicate(true);
						const Ref<FastNoiseLite> noise = parameters->get_noise();
						noise->set_offset(noise->get_offset() + Vector3{chunk_position.z, -chunk_position.x, 0.0f});

						thread->start(callable_mp_static(generate_terrain).bind(parameters));
					}

					if (generated_chunks.has(chunk_coordinates)) {
						MeshInstance3D* chunk = cast_to<MeshInstance3D>(generated_chunks[chunk_coordinates]);
						add_child(chunk, false, INTERNAL_MODE_BACK);
						visible_chunks.append(chunk);
					}
				}
			}
		}
	}
}

MeshInstance3D* ProceduralTerrain::generate_terrain(const Ref<ProceduralTerrainParameters>& parameters) {
	ERR_FAIL_NULL_V_MSG(parameters, nullptr, "ProceduralTerrainParameters must be non-mull.");
	ERR_FAIL_COND_V_MSG(!parameters->has_valid_subresources(), nullptr, "ProceduralTerrainParameters are missing one or more valid subresources.");

	MeshInstance3D* terrain = memnew(MeshInstance3D);
	
	const auto octaves = parameters->get_octaves();
	const auto noise = parameters->get_noise();
	const auto persistence = parameters->get_persistence();
	const auto lacunarity = parameters->get_lacunarity();
	const auto lod = parameters->get_level_of_detail();
	const auto curve = parameters->get_height_curve();
	const auto scale = parameters->get_height_scale();
	const auto flatshaded = parameters->get_flatshaded();
	const auto color_map = parameters->get_color_map();
	const auto mode = parameters->get_generation_mode();

	Array matrix;
	const Array falloff = generate_falloff(parameters->get_falloff());
	
	if (mode == ProceduralTerrainParameters::GenerationMode::GENERATION_MODE_FALLOFF) {
		matrix = falloff;
	} else {
		matrix = generate_matrix(octaves, noise, persistence, lacunarity);
		apply_falloff(matrix, falloff);
	}

	Ref<Mesh> mesh;
	
	if (mode == ProceduralTerrainParameters::GenerationMode::GENERATION_MODE_NORMAL) {
		mesh = flatshaded ?  generate_flatshaded_mesh(matrix, lod, curve, scale) : generate_mesh(matrix, lod, curve, scale);
	} else {
		const Ref<PlaneMesh> plane = memnew(PlaneMesh);
		plane->set_size(Vector2{CHUNK_SIZE, CHUNK_SIZE});
		mesh = plane;
	}
	
	const Ref<StandardMaterial3D> material = generate_material(matrix, color_map);

	terrain->set_material_override(material);
	terrain->set_mesh(mesh);
	
	return terrain;
}

Array ProceduralTerrain::generate_matrix(const int octaves, const Ref<FastNoiseLite>& noise, const real_t persistence, const real_t lacunarity) {
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
				const real_t sample_x = (-static_cast<real_t>(x) - HALF_MATRIX_SIZE + offset.x) * frequency;
				const real_t sample_y = (static_cast<real_t>(y) - HALF_MATRIX_SIZE + offset.y) * frequency;
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

Ref<Mesh> ProceduralTerrain::generate_mesh(const Array& matrix, const int level_of_detail, const Ref<Curve>& height_curve, const real_t height_scale) {
	const int increment = CLAMP((MAX_LEVEL_OF_DETAIL - level_of_detail) * 2, 1, 12);
	const int vertices_per_line = (MATRIX_SIZE - 1) / increment + 1;

	Array arrays{};
	arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertices{};
	PackedVector2Array uvs{};
	PackedInt32Array indices{};
	PackedVector3Array normals{};

	vertices.resize(pow(vertices_per_line, 2));
	uvs.resize(vertices.size());
	indices.resize(pow(vertices_per_line - 1, 2) * 6);
	normals.resize(vertices.size());
	
	int vertex_index = 0;
	int indices_index = 0;

	for (int y = 0; y < MATRIX_SIZE; y += increment) {
		for (int x = 0; x < MATRIX_SIZE; x += increment) {
			const real_t height = height_curve->sample(matrix[y * MATRIX_SIZE + x]) * height_scale;
			vertices.set(vertex_index, Vector3(static_cast<real_t>(x) - CENTER_OFFSET, height, static_cast<real_t>(y) - CENTER_OFFSET));
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

	
	Ref<ArrayMesh> mesh{};
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	return mesh;
}

Ref<Mesh> ProceduralTerrain::generate_flatshaded_mesh(const Array& matrix, int level_of_detail, const Ref<Curve>& height_curve, const real_t height_scale) {
	const int increment = CLAMP((MAX_LEVEL_OF_DETAIL - level_of_detail) * 2, 1, 12);
	const int vertices_per_line = (MATRIX_SIZE - 1) / increment + 1;
	
	Array arrays{};
	arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertices{};
	PackedVector2Array uvs{};
	PackedInt32Array indices{};
	PackedVector3Array normals{};

	indices.resize(pow(vertices_per_line - 1, 2) * 6);
	vertices.resize(indices.size());
	uvs.resize(vertices.size());
	normals.resize(vertices.size());

	int vertex_index = 0;

	for (int y = 0; y < MATRIX_SIZE; y += increment) {
		for (int x = 0; x < MATRIX_SIZE; x += increment) {
			if (x < MATRIX_SIZE - 1 && y < MATRIX_SIZE - 1) {
				Vector3 a {  // (x, y)
					x - CENTER_OFFSET,
					height_curve->sample(matrix[y * MATRIX_SIZE + x]) * height_scale,
					y - CENTER_OFFSET
				};
				Vector2 a_uv {
					static_cast<real_t>(x) / MATRIX_SIZE,
					static_cast<real_t>(y) / MATRIX_SIZE
				};
				Vector3 b {  // (x + inc, y)
					(x + increment) - CENTER_OFFSET,
					height_curve->sample(matrix[y * MATRIX_SIZE + (x + increment)]) * height_scale,
					y - CENTER_OFFSET
				};
				Vector2 b_uv {
					static_cast<real_t>(x + increment) / MATRIX_SIZE,
					static_cast<real_t>(y) / MATRIX_SIZE
				};
				Vector3 c {  // (x, y + inc)
					x - CENTER_OFFSET,
					height_curve->sample(matrix[(y + increment) * MATRIX_SIZE + x]) * height_scale,
					(y + increment) - CENTER_OFFSET
				};
				Vector2 c_uv {
					static_cast<real_t>(x) / MATRIX_SIZE,
					static_cast<real_t>(y + increment) / MATRIX_SIZE
				};
				Vector3 d {  // (x + inc , y + inc)
					(x + increment) - CENTER_OFFSET,
					height_curve->sample(matrix[(y + increment) * MATRIX_SIZE + (x + increment)]) * height_scale,
					(y + increment) - CENTER_OFFSET
				};
				Vector2 d_uv {
					static_cast<real_t>(x + increment) / MATRIX_SIZE,
					static_cast<real_t>(y + increment) / MATRIX_SIZE
				};

				uvs.set(vertex_index, a_uv);
				vertices.set(vertex_index++, a);
				uvs.set(vertex_index, d_uv);
				vertices.set(vertex_index++, d);
				uvs.set(vertex_index, c_uv);
				vertices.set(vertex_index++, c);

				uvs.set(vertex_index, d_uv);
				vertices.set(vertex_index++, d);
				uvs.set(vertex_index, a_uv);
				vertices.set(vertex_index++, a);
				uvs.set(vertex_index, b_uv);
				vertices.set(vertex_index++, b);
				
			}
		}
	}

	for (int i = 0; i < indices.size(); i++) {
		indices.set(i, i);
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
	
		normals.set(a_index, normal);
		normals.set(b_index, normal);
		normals.set(c_index, normal);
	}

	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_TEX_UV] = uvs;
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	
	Ref<ArrayMesh> mesh{};
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	return mesh;
}

Array ProceduralTerrain::generate_falloff(const Vector2 falloff) {
	Array map{};
	map.resize(MATRIX_SIZE * MATRIX_SIZE);
	int index = 0;

	if (falloff == Vector2{}) {
		map.fill(0);
		return map;
	}
	
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

void ProceduralTerrain::apply_falloff(Array matrix, const Array& falloff) {
	int index = 0;
	for (int y = 0; y < MATRIX_SIZE; y++) {
		for (int x = 0; x < MATRIX_SIZE; x++) {
			const real_t falloff_amount = falloff[index];
			const real_t previous_value = matrix[index];
			matrix[index] = previous_value - falloff_amount;
			index++;
		}
	}
}

Ref<StandardMaterial3D> ProceduralTerrain::generate_material(const Array& matrix, const Ref<Gradient>& color_map) {
	const Ref<Image> image = Image::create_empty(MATRIX_SIZE, MATRIX_SIZE, false, Image::FORMAT_RGB8);
	for (int y = 0; y < MATRIX_SIZE; y++) {
		for (int x = 0; x < MATRIX_SIZE; x++) {
			const real_t height = matrix[y * MATRIX_SIZE + x];
			Color color = color_map->get_color_at_offset(height);
			image->set_pixel(x, y, color);
		}
	}
	
	const Ref<Texture> texture = ImageTexture::create_from_image(image);
	const Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
	material->set_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO, texture);
	material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);

	return material;
}
