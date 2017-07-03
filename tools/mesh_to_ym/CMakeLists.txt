cmake_minimum_required(VERSION 3.6)
project(mesh_to_ym)

include_directories(${mesh_to_ym_SOURCE_DIR})

set(ULTIMATE_WARNINGS "-Wdisabled-optimization -Wlogical-op -Wwrite-strings -Wmissing-include-dirs -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef")
set(EXTRA_WARNINGS "-Woverloaded-virtual -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Wodr")
# no -Wzero-as-null-pointer-constant because of VK_NULL_HANDLE

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_definitions("${EXTRA_WARNINGS}")
add_definitions("${ULTIMATE_WARNINGS}")
add_definitions("-std=c++17 -pedantic -Wall -Wextra")



include_directories(../../)

set(SOURCE_FILES
		"main.cpp"

		"import.h"
		"import_mesh.cpp"
		"import_skeleton.cpp"
		"import_animation.cpp"
		"import_scene.cpp"

		"../../yave/meshes/MeshData.h"
		"../../yave/meshes/MeshData.cpp"

		"../../yave/animations/AnimationChannel.h"
		"../../yave/animations/AnimationChannel.cpp"

		"../../yave/animations/Animation.h"
		"../../yave/animations/Animation.cpp"
	)

# y lib, change this to wherever it it
include_directories(../../../y)
link_directories(../../../y/build)

add_executable(mesh_to_ym ${SOURCE_FILES})

target_link_libraries(mesh_to_ym y)
target_link_libraries(mesh_to_ym assimp.dll)





