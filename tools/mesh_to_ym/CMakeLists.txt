cmake_minimum_required(VERSION 3.6)
project(mesh_to_ym)

include_directories(${yave_SOURCE_DIR})
include_directories(${y_SOURCE_DIR})

set(SOURCE_FILES
		"main.cpp"

		"import.h"
		"import_mesh.cpp"
		"import_skeleton.cpp"
		"import_animation.cpp"
		"import_scene.cpp"

		"transforms.h"
		"transforms.cpp"
	)

add_executable(mesh_to_ym ${SOURCE_FILES})

target_link_libraries(mesh_to_ym y)
target_link_libraries(mesh_to_ym yave)
target_link_libraries(mesh_to_ym assimp.dll)





