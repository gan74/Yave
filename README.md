# Yave
Yet another C++20 Vulkan engine.

## Disclaimer
This is a pet/learning project, it's not meant to be a serious engine and should not be used for anything beside playing with Vulkan.

Currently only supports Windows
 * With GCC 14+ through MSYS2
 * With MSVC 19.28+

Serialization format is compiler dependent!

![Current status](https://i.imgur.com/LlveBf3.png)
Older UI: [1](https://i.imgur.com/ulnz4Rs.jpeg) [2](https://i.imgur.com/L8Awj4C.png)

## Project structure

 * y: Core library with a bunch of utility functions and classes
 * yave: The engine itself
    It links only to y and spirv_reflect
 * editor: A scene editor build on top of yave
 * shaders: All the shaders for both the engine and the editor
 * external: Third party libraries


## Building
You need:
 * CMake (3.25)
 * A C++20 compiler
 * [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
   * With Volk


Implemented features:
 * All basic Vulkan features
   * Buffers
   * Images
   * Arrays
   * Cubemaps and IBL probes
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
   * Shadows for spot and directional lights
   * Basic scenes
   * TAA
   * Bindless renderer with batching

### Licence:
MIT
