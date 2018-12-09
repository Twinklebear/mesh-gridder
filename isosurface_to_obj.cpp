#include <iostream>
#include <map>
#include <unordered_map>
#include <array>
#include <vector>
#include <fstream>
#include <regex>

#include <vtkSmartPointer.h>
#include <vtkTriangle.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>

#include "math.h"

int main(int argc, char **argv) {
	const int nisosurfaces = std::atoi(argv[3]);
	const std::string outputfile = argv[2];
	const std::string file = argv[1];
	const std::regex match_filename("(\\w+)_(\\d+)x(\\d+)x(\\d+)_(.+)\\.raw");
	auto matches = std::sregex_iterator(file.begin(), file.end(), match_filename);
	if (matches == std::sregex_iterator() || matches->size() != 6) {
		std::cerr << "Unrecognized raw volume naming scheme, expected a format like: "
			<< "'<name>_<X>x<Y>x<Z>_<data type>.raw' but '" << file << "' did not match"
			<< std::endl;
		throw std::runtime_error("Invalaid raw file naming scheme");
	}

	std::array<int, 3> dims{std::stoi((*matches)[2]),
		std::stoi((*matches)[3]),
		std::stoi((*matches)[4])};

	std::string data_type = (*matches)[5];

	size_t dtype_size = 0;
	int vtk_data_type = -1;
	if (data_type == "uint8") {
		dtype_size = 1;
		vtk_data_type = VTK_UNSIGNED_CHAR;
	} else if (data_type == "int8") {
		dtype_size = 1;
		vtk_data_type = VTK_CHAR;
	} else if (data_type == "uint16") {
		dtype_size = 2;
		vtk_data_type = VTK_UNSIGNED_SHORT;
	} else if (data_type == "int16") {
		dtype_size = 2;
		vtk_data_type = VTK_SHORT;
	} else if (data_type == "float32" || data_type == "float") {
		dtype_size = 4;
		vtk_data_type = VTK_FLOAT;
	} else if (data_type == "float64" || data_type == "double") {
		dtype_size = 8;
		vtk_data_type = VTK_DOUBLE;
	} else {
		throw std::runtime_error("Unsupported or unrecognized data type: " + data_type);
	}

	std::vector<char> volume_data(dtype_size * dims[0] * dims[1] * dims[2], 0);
	{
		std::ifstream fin(file.c_str(), std::ios::binary);
		fin.read(volume_data.data(), volume_data.size());
	}
	
	vtkSmartPointer<vtkImageData> img_data = vtkSmartPointer<vtkImageData>::New();
	img_data->SetDimensions(dims[0], dims[1], dims[2]);
	img_data->AllocateScalars(vtk_data_type, 1);
	std::memcpy(img_data->GetScalarPointer(), volume_data.data(), volume_data.size());

	vtkSmartPointer<vtkFlyingEdges3D> fedges = vtkSmartPointer<vtkFlyingEdges3D>::New();
	fedges->SetInputData(img_data);
	fedges->SetNumberOfContours(nisosurfaces);
	for (int i = 0; i < nisosurfaces; ++i) {
		std::cout << "Isoval: " << argv[i + 4] << "\n";
		fedges->SetValue(i, std::atof(argv[i + 4]));
	}
	fedges->SetComputeNormals(false);
	fedges->Update();
	vtkPolyData *isosurface = fedges->GetOutput();
	isosurface->PrintSelf(std::cout, vtkIndent());

	if (outputfile != "nooutput") {
		std::cout << "Saving mesh to " << outputfile << "\n";
		std::ofstream fout;
		const bool write_binary = outputfile.substr(outputfile.size() - 4) == "bobj";
		if (!write_binary) {
			fout.open(outputfile.c_str());
		} else {
			fout.open(outputfile.c_str(), std::ios::binary);
			uint64_t header[2] = {0};
			fout.write(reinterpret_cast<char*>(header), sizeof(header));
		}
		size_t next_vert_id = 1;
		std::unordered_map<uint64_t, uint64_t> vertex_remapping;
		std::map<vec3f, uint64_t> remapped_verts;

		for (size_t i = 0; i < isosurface->GetNumberOfCells(); ++i) {
			vtkTriangle *tri = dynamic_cast<vtkTriangle*>(isosurface->GetCell(i));
			if (tri->ComputeArea() == 0.0) {
				continue;
			}
			for (size_t v = 0; v < 3; ++v) {
				const vec3f vert(isosurface->GetPoint(tri->GetPointId(v))[0],
						isosurface->GetPoint(tri->GetPointId(v))[1],
						isosurface->GetPoint(tri->GetPointId(v))[2]);

				if (remapped_verts.find(vert) == remapped_verts.end()) {
					remapped_verts[vert] = next_vert_id;

					if (!write_binary) {
						fout << "v " << vert.x << " " << vert.y << " " << vert.z << "\n";
					} else {
						fout.write(reinterpret_cast<const char*>(&vert), sizeof(vert));
					}

					++next_vert_id;
				}
				vertex_remapping[tri->GetPointId(v)] = remapped_verts[vert];
			}
		}
		const uint64_t n_verts_written = next_vert_id - 1;
		uint64_t n_indices_written = 0;
		for (size_t i = 0; i < isosurface->GetNumberOfCells(); ++i) {
			std::array<uint64_t, 3> tids;
			vtkTriangle *tri = dynamic_cast<vtkTriangle*>(isosurface->GetCell(i));
			if (tri->ComputeArea() == 0.0) {
				continue;
			}
			for (size_t v = 0; v < 3; ++v) {
				tids[v] = vertex_remapping[tri->GetPointId(v)];
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

	return 0;
}

