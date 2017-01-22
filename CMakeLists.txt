cmake_minimum_required(VERSION 3.6)
project(y)

include_directories(${y_SOURCE_DIR})

set(ULTIMATE_WARNINGS "-Wdisabled-optimization -Wlogical-op -Wwrite-strings -Wmissing-include-dirs -Wnoexcept -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef")
set(EXTRA_WARNINGS "-Woverloaded-virtual -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Wodr -Wzero-as-null-pointer-constant")
set(EXTRA_OPTIMISATIONS "-ffast-math")

if(CMAKE_BUILD_TYPE MATCHES "Release")
	set(EXTRA_OPTIMISATIONS "${EXTRA_OPTIMISATIONS} -O3")
	message("Release build")
endif()


add_definitions("${EXTRA_WARNINGS}")
add_definitions("${ULTIMATE_WARNINGS}")
add_definitions("${EXTRA_OPTIMISATIONS}")
add_definitions("-std=c++1z -pedantic -Wall -Wextra")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_OPTIMISATIONS}")
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} ${EXTRA_OPTIMISATIONS}")


file(GLOB_RECURSE SOURCE_FILES
		"y/*.h"
		"y/*.cpp"
	)

file(GLOB_RECURSE TEST_FILES
		"tests/*.cpp"
	)

add_library(y STATIC ${SOURCE_FILES})

add_executable(main "main.cpp")
target_link_libraries(main y)

add_executable(tests ${TEST_FILES} "main.cpp")
target_compile_definitions(tests PRIVATE "-DY_BUILD_TESTS")
target_link_libraries(tests y)

