cmake_minimum_required(VERSION 3.7)
project(yave)

find_package(vulkan REQUIRED)

option(BUILD_TOOLS "Build tools" ON)
option(BUILD_EDITOR "Build editor" ON)

# add y subtree
add_subdirectory(y)

add_subdirectory(external/spirv_cross)

# setup includes
include_directories(${y_SOURCE_DIR})
include_directories(${yave_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(external)


# Yave's core file
file(GLOB_RECURSE YAVE_FILES
		"yave/*.h"
		"yave/*.cpp"
	)
	
# Editor files
file(GLOB_RECURSE EDITOR_FILES
		"editor/*.h"
		"editor/*.cpp"
		"external/imgui*.h"
		"external/imgui*.cpp"
		"external/imgui*.inl"
	)

# Shader files, they are here so the IDE can find them
file(GLOB_RECURSE SHADER_FILES
		"shaders/*.frag"
		"shaders/*.vert"
		"shaders/*.geom"
		"shaders/*.comp"
		"shaders/*.glsl"
	)
	
	
add_library(yave ${YAVE_FILES} ${SHADER_FILES})
target_link_libraries(yave ${Vulkan_LIBRARY})
target_link_libraries(yave y spirv-cross-core)


if(BUILD_EDITOR)
	add_executable(editor ${EDITOR_FILES})
	target_link_libraries(editor yave)
endif()


if(BUILD_TOOLS)
	add_subdirectory(tools)
endif()






