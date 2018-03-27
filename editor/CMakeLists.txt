cmake_minimum_required(VERSION 3.6)
project(editor)

include_directories(${editor_SOURCE_DIR})
include_directories(${yave_SOURCE_DIR})
include_directories(${y_SOURCE_DIR})

include_directories(external)

include_directories(${Vulkan_INCLUDE_DIRS})



file(GLOB_RECURSE SOURCE_FILES
		"*.h"
		"*.cpp"
		"*.inl"
	)

add_executable(editor ${SOURCE_FILES})

target_link_libraries(editor y)
target_link_libraries(editor yave)




