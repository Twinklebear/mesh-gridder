#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <array>
#include "tbb/tbb.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math.h"

void write_obj_brick(const tinyobj::attrib_t &attrib, const tinyobj::shape_t &shape,
		const std::vector<size_t> &tris, const std::string &fname);

int main(int argc, char **argv) {
	if (argc != 6 || std::strcmp(argv[1], "-h") == 0) {
		std::cout << "Usage: " << argv[0] << " <in.obj> <x> <y> <z> <output prefix>\n"
			<< "    The input OBJ file will be gridded onto an <x>*<y>*<z> grid\n"
			<< "    each grid cell will then be output as <output prefix>#.obj\n"
			<< "    where # indicates the grid cell id.\n";
	}

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

	box3f model_bounds;
	const auto &shape = shapes[0];
	for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
		int fv = shape.mesh.num_face_vertices[f];
		if (fv != 3) {
			std::cout << "Error: only triangle meshes are supported\n";
			return 1;
		}
		// Loop over vertices in the face.
		for (size_t v = 0; v < 3; ++v) {
			tinyobj::index_t idx = shape.mesh.indices[f * 3 + v];
			vec3f p(attrib.vertices[3*idx.vertex_index],
					attrib.vertices[3*idx.vertex_index+1],
					attrib.vertices[3*idx.vertex_index+2]);
			model_bounds.extend(p);
		}
	}
	
	// Setup grid structure (a list of which triangle IDs touch the cell)
	const vec3sz grid(std::atoll(argv[2]), std::atoll(argv[3]), std::atoll(argv[4]));
	const vec3f brick_size = (model_bounds.upper - model_bounds.lower) / vec3f(grid);
	const size_t ncells = grid.x * grid.y * grid.z;
	std::cout << "Bounds of model: " << model_bounds << "\n"
		<< "Grid to " << grid << " dim grid\n"
		<< "Brick size = " << brick_size << "\n";

	//tbb::parallel_for(size_t(0), ncells, size_t(1),
	for (size_t i = 0; i < ncells; ++i) {
		auto fn = [&](const size_t i) {
			const vec3sz idx(i % grid.x, (i / grid.x) % grid.y, i / (grid.x * grid.y));
			const vec3f blower(
					rescale_value(idx.x, 0, grid.x, model_bounds.lower.x, model_bounds.upper.x),
					rescale_value(idx.y, 0, grid.y, model_bounds.lower.y, model_bounds.upper.y),
					rescale_value(idx.z, 0, grid.z, model_bounds.lower.z, model_bounds.upper.z));
			const box3f brick_bounds(blower, blower + brick_size);
			std::cout << "Brick idx: " << idx << "\nBounds: " << brick_bounds << "\n";

			std::vector<size_t> contained_tris;
			// Loop through the mesh and see which triangles are contained in this grid cell
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
				std::array<vec3f, 3> tri;
				for (size_t v = 0; v < 3; ++v) {
					tinyobj::index_t idx = shape.mesh.indices[f * 3 + v];
					vec3f p(attrib.vertices[3*idx.vertex_index],
							attrib.vertices[3*idx.vertex_index+1],
							attrib.vertices[3*idx.vertex_index+2]);
					tri[v] = p;
				}
				std::cout << "triangle "
					<< tri[0] << ", " << tri[1] << ", " << tri[2];
				if (triangle_box_intersection(tri[0], tri[1], tri[2], brick_bounds)) {
					std::cout << ", is in\n";
					contained_tris.push_back(f);
				} else {
					std::cout << ", not in\n";
				}

			}
			// Need to now save out the OBJ files. To do so, we need to take
			// just the vertices that we have for the cell, remap the indices and write
			// out the file
			const std::string fname = argv[5] + std::to_string(i) + ".obj";
			write_obj_brick(attrib, shape, contained_tris, fname);
		};//);
		fn(i);
	}

	return 0;
}
void write_obj_brick(const tinyobj::attrib_t &attrib, const tinyobj::shape_t &shape,
		const std::vector<size_t> &tris, const std::string &fname)
{
	size_t next_vert_id = 1;
	std::unordered_map<size_t, size_t> vertex_remapping;
	for (const auto &t : tris) {
		for (size_t v = 0; v < 3; ++v) {
			tinyobj::index_t idx = shape.mesh.indices[t * 3 + v];
			if (vertex_remapping.find(idx.vertex_index) == vertex_remapping.end()) {
				vertex_remapping[idx.vertex_index] = next_vert_id;
				++next_vert_id;
			}
		}
	}
	std::ofstream fout(fname.c_str());
	for (const auto &v : vertex_remapping) {
		fout << "v " << attrib.vertices[3*v.first]
			<< " " << attrib.vertices[3*v.first + 1]
			<< " " << attrib.vertices[3*v.first + 2]
			<< "\n";
	}
	for (const auto &t : tris) {
		std::array<size_t, 3> tids;
		for (size_t v = 0; v < 3; ++v) {
			tinyobj::index_t idx = shape.mesh.indices[t * 3 + v];
			tids[v] = vertex_remapping[idx.vertex_index];
		}
		fout << "f " << tids[0] << " " << tids[1] << " " << tids[2] << "\n";
	}
}

