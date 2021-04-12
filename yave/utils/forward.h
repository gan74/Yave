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
struct InlineBlock;
class TimeQuery;
class OctreeNode;
struct Contants;
struct BufferCreateInfo;
class Camera;
class CmdBufferRegion;
struct FrameGraphMutableResourceId;
class FrameGraphResourceId;
class SpotLightComponent;
struct Monitor;
class SwapchainImage;
class ImageData;
class ImageFormat;
class ShaderModuleBase;
class AssetLoadingContext;
struct LoadableComponentTypeInfo;
class LoaderBase;
class DescriptorSetLayout;
class InlineDescriptor;
class Renderable;
class Queue;
class DescriptorSetBase;
struct ImageCreateInfo;
struct SubPassData;
class ImageBarrier;
class Animation;
class RenderPassRecorder;
class TransformableComponent;
class FileSystemModel;
class Window;
struct ToneMappingPass;
struct FrameToken;
class Mapping;
struct ResourceCreateInfo;
struct Viewport;
struct BlurPass;
class EventHandler;
class GraphicPipeline;
class SearchableFileSystemModel;
class RenderPass;
struct BloomPass;
class SpirVData;
struct SubMesh;
struct BloomSettings;
struct Bone;
struct Attachment;
class MaterialTemplate;
struct CmdBuffer;
struct LightingSettings;
class SubBufferBase;
struct FrameGraphMutableImageId;
class ResourceFence;
class FrameGraphFrameResources;
class FrameGraphResourcePool;
struct Vertex;
struct SSAOPass;
class DeviceResources;
class PhysicalDevice;
class SceneView;
class LoadingJob;
class TransientBuffer;
class ShaderProgram;
class SQLiteFileSystemModel;
class AssetIdFactory;
class LifetimeManager;
class Swapchain;
struct ShadowMapSettings;
class PointLightComponent;
class AccelerationStructure;
struct InlineStorage;
class DeviceMemoryHeap;
struct DefaultRenderer;
struct Semaphores;
struct SkinWeights;
struct FrameGraphMutableBufferId;
class FrameGraphRegion;
class Frustum;
class DeviceMemoryAllocator;
struct Box;
class FrameGraph;
class LocalFileSystemModel;
struct AttachmentData;
class AssetLoader;
struct Region;
class FolderFileSystemModel;
class DeviceMemoryView;
class AABB;
class ThreadLocalDevice;
struct ResourceUsageInfo;
class DescriptorSetPool;
struct DownsamplePass;
struct SkeletonData;
class Layout;
class ComputeProgram;
class AssetLoaderSystem;
class CmdBufferRecorder;
class DedicatedDeviceMemoryAllocator;
struct ShadowMapPass;
struct AssetData;
class Octree;
class Instance;
struct ToneMappingSettings;
struct AtmospherePass;
class Framebuffer;
class RayTracing;
struct KeepAlive;
class AtmosphereComponent;
class Device;
struct ImageCopyInfo;
class SQLiteAssetStore;
class MaterialTemplateData;
struct Registerer;
class FolderAssetStore;
class DescriptorSetAllocator;
class MeshData;
struct BlurSettings;
class SpecializationData;
struct SubPass;
class IBLProbe;
struct EmptyResource;
class PushConstant;
class CmdBufferPool;
class ImageBase;
class FrameGraphPassBuilder;
class CmdBufferData;
class SimpleMaterialData;
struct LoadableComponentRegister;
struct RendererSettings;
struct Attribute;
struct LayoutPools;
struct FrameGraphBufferId;
class DebugUtils;
struct SSAOSettings;
class DescriptorSetData;
class SkyLightComponent;
struct GBufferPass;
class SkinnedMesh;
class OctreeSystem;
class FrameGraphPass;
class AssetDependencies;
struct BoneKey;
struct AssetId;
struct EnclosingSphere;
class BufferBarrier;
class BufferBase;
struct SkinnedVertex;
class AssetLoadingThreadPool;
class SkeletonInstance;
class DirectionalLightComponent;
class Sampler;
class Descriptor;
struct DeviceProperties;
class MaterialCompiler;
class DeviceMemoryHeapBase;
struct SceneRenderSubPass;
struct FrameGraphImageId;
class StaticMesh;
struct SceneData;
struct LightingPass;
class Material;
class DescriptorSet;
class AssetStore;
class AnimationChannel;
class GenericAssetPtr;
class Skeleton;
class FrameGraphDescriptorBinding;
class DebugParams;
class DeviceMemory;
struct FreeBlock;
struct BoneTransform;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkStructInitializer;
struct VkNull;
}


namespace yave::ecs {
class EntityWorld;
class ComponentContainerBase;
struct IdComponentsReturnPolicy;
class EntityPrefab;
class EntityIdPool;
class Archetype;
class IdComponents;
class System;
struct ComponentsReturnPolicy;
class SparseIdSet;
class EntityId;
struct ComponentRuntimeInfo;
struct IdReturnPolicy;
class ComponentBoxBase;
}


namespace yave::uniform {
struct Camera;
struct DirectionalLight;
struct RayleighSky;
struct PointLight;
struct ShadowMapParams;
struct SH;
struct SpotLight;
struct Surfel;
struct ToneMappingParams;
}


#endif // YAVE_UTILS_FORWARD_H