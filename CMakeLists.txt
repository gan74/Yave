cmake_minimum_required(VERSION 2.8)
project(n)

# SRC
file(GLOB SRC ./*.cpp
              ./n/*.cpp
              ./n/core/*.cpp
              ./n/assets/*.cpp
              ./n/concurrent/*.cpp
              ./n/graphics/*.cpp
              ./n/io/*.cpp
              ./n/math/*.cpp
              ./n/mem/*.cpp
              ./n/script/*.cpp
              ./n/signals/*.cpp
              ./n/test/*.cpp

              ./*.h
              ./n/*.h
              ./n/core/*.h
              ./n/assets/*.h
              ./n/concurrent/*.h
              ./n/graphics/*.h
              ./n/io/*.h
              ./n/math/*.h
              ./n/mem/*.h
              ./n/script/*.h
              ./n/signals/*.h
              ./n/test/*.h)

# STATIC LIBS
aux_source_directory(./dependencies/lodepng SRC)
aux_source_directory(./dependencies/glew SRC)

include_directories(.)

add_executable(n ${SRC})

#LIBS
link_directories("C:/mingw/mingw32/lib")
target_link_libraries(n opengl32 -lpthread -lmingw32 -lSDL2main -lSDL2.dll)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic -Winline")
set(CMAKE_CXX_FLAGS "-DN_DEBUG -DGLEW_STATIC -DN_USE_SDL_IMAGE ${CMAKE_CXX_FLAGS}")

#set(CMAKE_CXX_FLAGS_DEBUG "-DN_AUTO_TEST ${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS_RELEASE "-o3 -flto ${CMAKE_CXX_FLAGS}")

#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
#set(CMAKE_CXX_FLAGS "-DWIN32 ${CMAKE_CXX_FLAGS}")

