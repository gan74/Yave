/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_UTILS_FORWARD_H
#define YAVE_UTILS_FORWARD_H

// Auto generated: forward definitions for most non template classes
// TODO: Nested classes will not be declared correctly

namespace yave {
class ShaderModuleBase;
class BufferBase;
class Frustum;
class RenderPassRecorder;
struct BoneTransform;
class CmdBufferPool;
class Renderable;
class Mapping;
class RayTracing;
class AssetStore;
struct FreeBlock;
struct Box;
struct EmptyResource;
class DirectionalLightComponent;
struct ImageCopyInfo;
class DescriptorSetPool;
struct Viewport;
struct SceneRenderSubPass;
class IBLProbe;
class ResourceFence;
class OctreeData;
class LoaderBase;
class AssetLoadingContext;
class Animation;
struct BloomPass;
class GenericAssetPtr;
struct SceneData;
class FrameGraphDescriptorBinding;
struct InlineStorage;
class BufferBarrier;
struct FrameGraphBufferId;
struct FrameToken;
class SkeletonInstance;
class Queue;
class PointLightComponent;
struct EnclosingSphere;
class DeviceMemoryHeapBase;
class FileSystemModel;
class DeviceResources;
class OctreeSystem;
class Camera;
class CmdBufferRecorder;
struct SubMesh;
struct LightingSettings;
class SkyLightComponent;
struct ToneMappingSettings;
class SceneView;
class AssetIdFactory;
struct AssetDesc;
class MaterialTemplateData;
class InlineDescriptor;
class DeviceMemoryHeap;
struct LoadableComponentTypeInfo;
class FrameGraphRegion;
struct InlineBlock;
class SpirVData;
class SwapchainImage;
class MaterialTemplate;
class PushConstant;
class ImageBarrier;
struct FrameGraphMutableResourceId;
class MeshData;
class DescriptorSetAllocator;
class AtmosphereComponent;
class AssetLoader;
class DeviceMemoryView;
struct SkinnedVertex;
class FrameGraphPass;
struct LoadableComponentRegister;
struct ResourceCreateInfo;
class CmdBufferRegion;
struct DefaultRenderer;
class DescriptorSetBase;
class DescriptorSet;
class DedicatedDeviceMemoryAllocator;
class Window;
class MaterialCompiler;
class SkinnedMesh;
struct DownsamplePass;
struct BoneKey;
struct Semaphores;
struct RendererSettings;
class AssetDependencies;
class ShaderProgram;
struct SkinWeights;
struct LayoutPools;
struct Contants;
struct Registerer;
struct AssetData;
class StaticMesh;
struct Attribute;
class Layout;
struct SSAOPass;
class FrameGraphResourcePool;
class Octree;
class AccelerationStructure;
class LocalFileSystemModel;
struct Region;
class TransformableComponent;
struct SubPassData;
struct LightingPass;
class AssetLoadingThreadPool;
struct GBufferPass;
struct BlurPass;
class ImageFormat;
class DescriptorSetLayout;
class FolderAssetStore;
class DeviceMemoryAllocator;
struct ToneMappingPass;
class DebugParams;
class ImageBase;
class AABB;
struct AssetId;
struct FrameGraphImageId;
class Instance;
class Swapchain;
class SpecializationData;
struct BloomSettings;
class LoadingJob;
class FrameGraph;
struct Vertex;
class FrameGraphResourceId;
struct SSAOSettings;
class TransientBuffer;
class PhysicalDevice;
struct AtmospherePass;
class Framebuffer;
class TimeQuery;
class Sampler;
struct BufferCreateInfo;
class ImageData;
class FrameGraphPassBuilder;
struct SubPass;
class LifetimeManager;
class Descriptor;
struct Monitor;
class EventHandler;
class OctreeNode;
class ThreadLocalDevice;
class RenderPass;
struct Bone;
struct ImageCreateInfo;
class ComputeProgram;
class Material;
class FrameGraphFrameResources;
struct KeepAlive;
struct ShadowMapSettings;
struct Attachment;
struct AttachmentData;
class SimpleMaterialData;
class Device;
class SubBufferBase;
struct CmdBuffer;
class CmdBufferData;
class GraphicPipeline;
class SpotLightComponent;
class DescriptorSetData;
class Skeleton;
struct FrameGraphMutableImageId;
struct BlurSettings;
class DeviceMemory;
struct DeviceProperties;
class AnimationChannel;
struct ShadowMapPass;
class SearchableFileSystemModel;
struct ResourceUsageInfo;
class AssetLoaderSystem;
struct SkeletonData;
class DebugUtils;
class FolderFileSystemModel;
struct FrameGraphMutableBufferId;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkNull;
struct VkStructInitializer;
}


namespace yave::ecs {
class EntityId;
class EntityPrefab;
class ComponentContainerBase;
class System;
class Archetype;
class IdComponents;
struct ComponentsReturnPolicy;
class ComponentBoxBase;
struct ComponentRuntimeInfo;
class EntityIdPool;
struct IdComponentsReturnPolicy;
struct IdReturnPolicy;
class EntityWorld;
class SparseIdSet;
}


namespace yave::uniform {
struct SH;
struct DirectionalLight;
struct PointLight;
struct ToneMappingParams;
struct RayleighSky;
struct SpotLight;
struct Camera;
struct Surfel;
struct ShadowMapParams;
}


#endif // YAVE_UTILS_FORWARD_H