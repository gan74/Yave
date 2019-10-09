# Yave
Yet another C++17 Vulkan engine.

## Disclaimer
This is a pet/learning project, it's not meant to be a serious engine and should not be used for anything beside playing with vulkan.



## Status

Currently a mess, should smooth out as I am learning Vulkan.

The project is in its very early stages.
For now I am focussing on creating a decent Vulkan wrapper/interface and a small scene editor before moving to the real engine work.

![Current status](https://i.imgur.com/fLydq3W.png)
![Current status](https://i.imgur.com/TaJzCya.gif)
![Current status](https://i.imgur.com/NsngKS3.png)

The editor is currently unusable but I am working on it.

## Project structure

 * y: Core library with a bunch of utility functions and classes
 * yave: The engine itself
	It links only to y and spirv-cross
 * editor: A scene editor build on top of yave
 * shaders: All the shaders for both the engine and the editor
 * external: Third party libraries


## Building
You need:
 * CMake (3.7)
 * A C++17 compiler (such as GCC 9.2)
 * [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
 * For tools:
   * [Assimp](http://assimp.sourceforge.net/)



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
- [ ] Rename ArrayView to Span and MutableSpan
- [ ] DenseMap (Is SparseVector enough?)
- [ ] Basic reflection
- [ ] More robust serialization

### Framegraph
- [X] Barriers
- [X] Reuse resources over frames
- [ ] Resource aliasing to avoid copies
- [ ] Merge mapped buffers

### Engine
- [X] Asset saving and loading
- [X] Integrate ECS
- [X] Switch to VK_EXT_debug_utils
- [X] Sky
- [ ] Make assets easily serializable
- [ ] Culling
- [ ] Batching system
- [ ] Streaming
- [ ] Proper material system
- [ ] Shadows
- [ ] GI
- [ ] Spot lights
- [ ] Actual tone mapping

### Editor
- [X] Integrate ECS
- [X] Open several views
- [X] Rotation 
- [X] Create materials 
- [X] Editor only entities 
- [ ] Save open widgets
- [ ] Scale 
- [ ] Edit materials
- [ ] Prefabs
- [ ] Better save system
- [ ] Undo/redo
- [ ] Integrated profiler
- [ ] Edit shaders?
- [ ] Edit or create meshes?


