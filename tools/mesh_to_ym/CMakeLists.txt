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



file(GLOB_RECURSE SOURCE_FILES
		"main.cpp"
	)

# y lib, change this to wherever it it
include_directories(../../../y)
link_directories(../../../y/build)


add_executable(mesh_to_ym "main.cpp" ${SOURCE_FILES})

if(WIN32)
	target_compile_definitions(mesh_to_ym PRIVATE -DWIN32)
endif()

target_link_libraries(mesh_to_ym y)
target_link_libraries(mesh_to_ym assimp.dll)





