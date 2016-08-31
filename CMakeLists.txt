cmake_minimum_required(VERSION 3.6)
project(y)

include_directories(${y_SOURCE_DIR})

set(ULTIMATE_WARNINGS, "-Wconversion -Weffc++ -Wshadow -Wfloat-equal")
set(EXTRA_WARNINGS "-pedantic -Wall -Wextra -Woverloaded-virtual -Wwrite-strings -Wpointer-arith -Wcast-qual -Wcast-align -Wunreachable-code -Wold-style-cast")

add_definitions("${EXTRA_WARNINGS} ${ULTIMATE_WARNINGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -ffast-math -pedantic")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB SOURCE_FILES
		"y/*"
		"y/utils/*"
		"y/core/*"
		"y/math/*"
		"y/test/*"
		)

add_library(y STATIC ${SOURCE_FILES})

add_executable(main ${SOURCE_FILES} "main.cpp")
target_link_libraries(main y)

target_compile_definitions(main PRIVATE -DY_AUTO_TEST)
