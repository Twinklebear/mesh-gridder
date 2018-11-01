cmake_minimum_required(VERSION 3.5)
project(mesh_gridder)

list(APPEND CMAKE_MODULE_PATH "${mesh_gridder_SOURCE_DIR}/cmake")

find_package(TBB REQUIRED)

add_executable(mesh_gridder gridder.cpp math.cpp)
set_target_properties(mesh_gridder PROPERTIES CXX_STANDARD 14)
target_include_directories(mesh_gridder PUBLIC ${TBB_INCLUDE_DIRS})
target_compile_definitions(mesh_gridder PUBLIC ${TBB_DEFINITIONS})
target_link_libraries(mesh_gridder PUBLIC ${TBB_LIBRARIES})

option(ISOSURFACE_WRITER "Build the Isosurface to OBJ writer tool" ON)
if (ISOSURFACE_WRITER)
	find_package(VTK REQUIRED)

	add_executable(isosurface_to_obj isosurface_to_obj.cpp math.cpp)
	set_target_properties(isosurface_to_obj PROPERTIES CXX_STANDARD 14)
	target_link_libraries(isosurface_to_obj PUBLIC ${VTK_LIBRARIES})
endif()

