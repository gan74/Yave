cmake_minimum_required(VERSION 3.6)
project(yave)


include_directories(${yave_SOURCE_DIR})
include_directories(${y_SOURCE_DIR})

include_directories(${Vulkan_INCLUDE_DIRS})



# main files
file(GLOB_RECURSE SOURCE_FILES
		"yave/*.h"
		"yave/*.cpp"
	)

# Shader files, they are here so the IDE can find them
file(GLOB_RECURSE SHADER_FILES
		"shaders/*.frag"
		"shaders/*.vert"
		"shaders/*.geom"
		"shaders/*.comp"
		"shaders/*.glsl"
	)

	
	
add_library(yave STATIC ${SOURCE_FILES} ${SHADER_FILES})

target_link_libraries(yave y)
target_link_libraries(yave ${Vulkan_LIBRARY})
target_link_libraries(yave spirv-cross)

