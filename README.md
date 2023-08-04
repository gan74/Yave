# Yave
Yet another C++17 Vulkan engine.

## Disclaimer
This is a pet/learning project, it's not meant to be a serious engine and should not be used for anything beside playing with Vulkan.

Currently only supports Windows
 * With GCC 9+ through MSYS2
 * With MSVC 16.9+

Serialization format is compiler dependent!

## Status

Currently a mess, should smooth out as I am learning Vulkan.

The project is in its  early stages.

![Current status](https://i.imgur.com/L8Awj4C.png)
[Older UI](https://i.imgur.com/ulnz4Rs.jpeg)

## Project structure

 * y: Core library with a bunch of utility functions and classes
 * yave: The engine itself
    It links only to y and spirv-cross
 * editor: A scene editor build on top of yave
 * shaders: All the shaders for both the engine and the editor
 * external: Third party libraries


## Building
You need:
 * CMake (3.24)
 * A C++17 compiler
 * [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
   * With Volk


Implemented features:
 * Buffers
 * Images
   * Arrays
   * Cubemaps
     * IBL probes
 * Descriptor sets
 * Basic pipelines
 * Compute shaders
 * Swapchain
 * Framebuffers
 * Resources lifetime management
 * Framegraph
 * Rendering pipeline
   * Tiled deferred shader
     * Physically based lighting
     * [IBL](https://i.imgur.com/fLydq3W.png)
   * Basic scenes
 * Meshes
   * Static
   * [Skeletal](https://i.imgur.com/TaJzCya.gif) 


### Licence:
MIT
