# Yave
Yet another C++17 Vulkan engine.

## Status

![Current status](https://i.imgur.com/fLydq3W.png)
![Current status](https://i.imgur.com/TaJzCya.gif)

## Project structure

 * y: Core library with a bunch of utility functions and classes
 * yave: The engine itself
	It links only to y and spirv-cross
 * editor: A scene editor build on top of yave
 * shaders: All the shaders for both the engine and the editor
 * external: Third party libraries
 * tools: Tools to create yave assets, should be moved into the editor soon 


## Building
You need:
 * CMake (3.7)
 * A C++17 compiler (GCC 7.1)
 * [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
 * For tools:
   * [Rust](https://www.rust-lang.org/en-US/)
   * [Assimp](http://assimp.sourceforge.net/)




Currently a mess, should smooth out as I am learning Vulkan.

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
 * Rendering pipeline
   * Tiled deferred shader
     * Physically based lighting
     * [IBL](https://i.imgur.com/fLydq3W.png)
   * Basic scenes
 * Meshes
   * Static
   * [Skeletal](https://im3.ezgif.com/tmp/ezgif-3-fd5d083cba.gif) 


### Licence:
MIT
