cmake_minimum_required(VERSION 3.6)
project(yave)

include_directories(${yave_SOURCE_DIR})

include_directories(../y)

set(ULTIMATE_WARNINGS "-Wdisabled-optimization -Wlogical-op -Wwrite-strings -Wmissing-include-dirs -Wnoexcept -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef")
set(EXTRA_WARNINGS "-Woverloaded-virtual -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Wodr")
set(PROFILE_FLAGS "-pg -fno-omit-frame-pointer -gdwarf-2")
# no -Wzero-as-null-pointer-constant because of VK_NULL_HANDLE

set(EXTRA_OPTIMISATIONS "-ffast-math")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(EXTRA_OPTIMISATIONS "${EXTRA_OPTIMISATIONS} -flto")
	message("Release: LTO enabled")
# else()
# 	set(EXTRA_OPTIMISATIONS "${EXTRA_OPTIMISATIONS} ${PROFILE_FLAGS}")
# 	message("Debug: profiling enabled")
endif()

add_definitions("${EXTRA_WARNINGS}")
add_definitions("${ULTIMATE_WARNINGS}")
add_definitions("${EXTRA_OPTIMISATIONS}")
add_definitions("-std=c++1z -pedantic -Wall -Wextra")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_OPTIMISATIONS}")


file(GLOB_RECURSE SOURCE_FILES
		"main.cpp"
		"YaveApp.cpp"
		"YaveApp.h"

		"yave/*.h"
		"yave/*.cpp"

		"shaders/*.frag"
		"shaders/*.vert"
		"shaders/*.geom"
		)


link_directories(../y/build)
add_executable(yave ${SOURCE_FILES})
target_link_libraries(yave y)
target_link_libraries(yave vulkan-1)
target_link_libraries(yave spirv-cross)

if(WIN32)
target_compile_definitions(yave PRIVATE -DWIN32)
endif()


