#include <iostream>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>
#include <array>
#include "tbb/tbb.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math.h"

void write_obj_brick(const std::vector<float> &verts, const std::vector<uint64_t> indices,
		const std::vector<size_t> &tris, const std::string &fname, const bool write_binary);

int main(int argc, char **argv) {
	if (argc != 6 || std::strcmp(argv[1], "-h") == 0) {
		std::cout << "Usage: " << argv[0] << " <in.obj> <x> <y> <z> <output prefix>\n"
			<< "    The input OBJ file will be gridded onto an <x>*<y>*<z> grid\n"
			<< "    each grid cell will then be output as <output prefix>#.obj\n"
			<< "    where # indicates the grid cell id.\n";
	}

	const std::string infile = argv[1];
	const bool write_binary = infile.substr(infile.size() - 4) == "bobj";

    std::vector<uint64_t> indices;
    std::vector<float> verts;
	if (!write_binary) {
		// Load the OBJ file
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, argv[1]);
		if (!ret) {
			std::cout << "Error loading mesh: " << err << std::endl;
			return 1;
		}

		if (shapes.size() > 1) {
			std::cout << "Error: OBJ file must contain a single object/group\n";
			return 1;
		}

		const auto &shape = shapes[0];
		// Need to build the index buffer ourselves
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
			int fv = shape.mesh.num_face_vertices[f];
			if (fv != 3) {
				std::cout << "Error: only triangle meshes are supported\n" << std::flush;
				std::exit(1);
			}
			// Loop over vertices in the face.
			for (size_t v = 0; v < 3; ++v) {
				indices.push_back(shape.mesh.indices[f * 3 + v].vertex_index);
			}
		}
		verts = std::move(attrib.vertices);
	} else {
      std::ifstream fin(infile.c_str(), std::ios::binary);
      uint64_t header[2] = {0};
      fin.read(reinterpret_cast<char*>(header), sizeof(header));
      verts.resize(header[0] * 3, 0.f);
      fin.read(reinterpret_cast<char*>(verts.data()), sizeof(float) * 3 * header[0]);
	  indices.resize(header[1] * 3, 0);
      fin.read(reinterpret_cast<char*>(indices.data()), sizeof(uint64_t) * 3 * header[1]);
	}

	box3f model_bounds;
	for (size_t i = 0; i < verts.size() / 3; ++i) {
		vec3f p(verts[3 * i], verts[3 * i + 1], verts[3 * i + 2]);
		model_bounds.extend(p);
	}

	// Setup grid structure (a list of which triangle IDs touch the cell)
	const vec3sz grid(std::atoll(argv[2]), std::atoll(argv[3]), std::atoll(argv[4]));
	const vec3f brick_size = (model_bounds.upper - model_bounds.lower) / vec3f(grid);
	const size_t ncells = grid.x * grid.y * grid.z;
	std::cout << "Bounds of model: " << model_bounds << "\n"
		<< "Grid to " << grid << " dim grid\n"
		<< "Brick size = " << brick_size << "\n";

	tbb::parallel_for(size_t(0), ncells, size_t(1),
		[&](const size_t i) {
			const vec3sz idx(i % grid.x, (i / grid.x) % grid.y, i / (grid.x * grid.y));
			const vec3f blower(
					rescale_value(idx.x, 0, grid.x, model_bounds.lower.x, model_bounds.upper.x),
					rescale_value(idx.y, 0, grid.y, model_bounds.lower.y, model_bounds.upper.y),
					rescale_value(idx.z, 0, grid.z, model_bounds.lower.z, model_bounds.upper.z));
			const box3f brick_bounds(blower, blower + brick_size);

			std::vector<size_t> contained_tris;
			// Loop through the mesh and see which triangles are contained in this grid cell
			for (size_t f = 0; f < indices.size() / 3; ++f) {
				std::array<vec3f, 3> tri;
				for (size_t v = 0; v < 3; ++v) {
					tri[v].x = verts[3 * indices[3 * f + v]];
					tri[v].y = verts[3 * indices[3 * f + v] + 1];
					tri[v].z = verts[3 * indices[3 * f + v] + 2];
				}
				if (triangle_box_intersection(tri[0], tri[1], tri[2], brick_bounds)) {
					contained_tris.push_back(f);
				}

			}
			// Need to now save out the OBJ files. To do so, we need to take
			// just the vertices that we have for the cell, remap the indices and write
			// out the file
			std::string fname = argv[5] + std::to_string(i);
			if (!write_binary) {
				fname += ".obj";
			} else {
				fname += ".bobj";
			}
			write_obj_brick(verts, indices, contained_tris, fname, write_binary);
		});

	return 0;
}
void write_obj_brick(const std::vector<float> &verts, const std::vector<uint64_t> indices,
		const std::vector<size_t> &tris, const std::string &fname, const bool write_binary)
{
	std::ofstream fout;
	if (!write_binary) {
		fout.open(fname.c_str());
	} else {
		fout.open(fname.c_str(), std::ios::binary);
		uint64_t header[2] = {0};
		fout.write(reinterpret_cast<char*>(header), sizeof(header));
	}
	size_t next_vert_id = 1;
	std::unordered_map<uint64_t, uint64_t> vertex_remapping;
	std::map<vec3f, uint64_t> remapped_verts;

	for (const auto &t : tris) {
		for (size_t v = 0; v < 3; ++v) {
			const uint64_t vert_idx = indices[3 * t + v];
			vec3f vert(verts[3 * vert_idx], verts[3 * vert_idx + 1], verts[3 * vert_idx + 2]);

			if (remapped_verts.find(vert) == remapped_verts.end()) {
				remapped_verts[vert] = next_vert_id;

				if (!write_binary) {
					fout << "v " << vert.x << " " << vert.y << " " << vert.z << "\n";
				} else {
					fout.write(reinterpret_cast<const char*>(&vert), sizeof(vert));
				}

				++next_vert_id;
			}
			vertex_remapping[vert_idx] = remapped_verts[vert];
		}
	}
	const uint64_t n_verts_written = next_vert_id - 1;
	uint64_t n_indices_written = 0;
	for (const auto &t : tris) {
		std::array<uint64_t, 3> tids;
		for (size_t v = 0; v < 3; ++v) {
			const uint64_t vert_idx = indices[3 * t + v];
			tids[v] = vertex_remapping[vert_idx];
		}
		if (!write_binary) {
			fout << "f " << tids[0] << " " << tids[1] << " " << tids[2] << "\n";
		} else {
			for (uint64_t &x : tids) {
				x -= 1;
			}
			fout.write(reinterpret_cast<char*>(tids.data()), sizeof(uint64_t) * tids.size());
		}
		++n_indices_written;
	}
	// Seek back and update the header
	if (write_binary) {
		fout.seekp(0);
		fout.write(reinterpret_cast<const char*>(&n_verts_written), sizeof(uint64_t));
		fout.write(reinterpret_cast<char*>(&n_indices_written), sizeof(uint64_t));
	}
}

