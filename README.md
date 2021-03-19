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

The project is in its very early stages.
For now I am focussing on creating a decent Vulkan wrapper/interface and a small scene editor before moving to the real engine work.

![Current status](https://i.imgur.com/fLydq3W.png)
![Current status](https://i.imgur.com/TaJzCya.gif)
![Current status](https://i.imgur.com/NsngKS3.png)
![Current status](https://i.imgur.com/fYBbB80.png)

## Project structure

 * y: Core library with a bunch of utility functions and classes
 * yave: The engine itself
    It links only to y and spirv-cross
 * editor: A scene editor build on top of yave
 * shaders: All the shaders for both the engine and the editor
 * external: Third party libraries


## Building
You need:
 * CMake (3.18)
 * A C++17 compiler
 * [Vulkan SDK](https://lunarg.com/vulkan-sdk/)


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
   * [Skeletal](https://im3.ezgif.com/tmp/ezgif-3-fd5d083cba.gif) 


### Licence:
MIT

## To Do


### Y
- [X] Log callbacks
- [ ] Finish allocators
- [ ] Rewrite SmallVector (with new allocators)
- [X] Rename ArrayView to Span and MutableSpan
- [X] HashMap
- [X] Basic reflection
- [X] More robust serialization

### Framegraph
- [X] Barriers
- [X] Reuse resources over frames
- [X] Resource aliasing to avoid copies
- [ ] Merge mapped buffers
- [ ] Better usage of render passes

### Engine
- [X] Asset saving and loading
- [X] Integrate ECS
- [X] Switch to VK_EXT_debug_utils
- [ ] Sky
- [ ] Make assets easily serializable
- [ ] Culling
- [ ] Batching system
- [ ] Streaming
- [ ] Proper material system
- [X] Shadows
- [ ] GI
- [ ] Post processes
- [X] Spot lights
- [X] Actual tone mapping

### Editor
- [X] Integrate ECS
- [X] Open several views
- [X] Rotation 
- [X] Create materials 
- [X] Editor only entities 
- [ ] Save open widgets
- [ ] Scale 
- [ ] Edit materials
- [X] Prefabs
- [ ] Better save system
- [ ] Undo/redo
- [ ] Integrated profiler
- [ ] Edit shaders?
- [ ] Edit or create meshes?


