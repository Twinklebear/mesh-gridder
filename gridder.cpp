#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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
	
	// Setup grid structure (a list of which triangle IDs touch the cell)
	// in parallel for each grid cell:
	//   loop through the triangles and find those which intersect the cell
	//   add the intersecting triangle ids to the list of tris in the cell
	//   write out each cell in some binary format to load them quickly

	return 0;
}

