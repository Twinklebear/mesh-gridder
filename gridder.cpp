#include <iostream>
#include <array>
#include "tbb/tbb.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math.h"

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

	tbb::parallel_for(size_t(0), ncells, size_t(1),
		[&](const size_t i) {
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
				if (triangle_box_intersection(tri[0], tri[1], tri[2], brick_bounds)) {
					std::cout << "triangle "
						<< tri[0] << ", " << tri[1] << ", " << tri[2]
						<< ", is in " << brick_bounds << "\n";
					contained_tris.push_back(f);
				}
			}
		});

	// in parallel for each grid cell:
	//   loop through the triangles and find those which intersect the cell
	//   add the intersecting triangle ids to the list of tris in the cell
	//   write out each cell in some binary format to load them quickly

	return 0;
}

