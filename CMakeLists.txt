cmake_minimum_required(VERSION 3.5)
project(mesh_gridder)

list(APPEND CMAKE_MODULE_PATH "${mesh_gridder_SOURCE_DIR}/cmake")

find_package(TBB REQUIRED)

add_executable(mesh_gridder gridder.cpp math.cpp)
target_include_directories(mesh_gridder PRIVATE ${TBB_INCLUDE_DIRS})
target_compile_definitions(mesh_gridder PRIVATE ${TBB_DEFINITIONS})
target_link_libraries(mesh_gridder ${TBB_LIBRARIES})

