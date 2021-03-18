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
class SearchableFileSystemModel;
class DeviceMemoryHeapBase;
class CmdBufferRegion;
struct ToneMappingPass;
class TransientBuffer;
struct FrameGraphBufferId;
class FrameGraphResourceId;
class RenderPass;
struct FrameGraphMutableResourceId;
class DirectionalLightComponent;
struct SkeletonData;
class RayTracing;
struct Viewport;
class DebugParams;
struct Attachment;
struct SSAOSettings;
class Descriptor;
class MaterialCompiler;
class RenderPassRecorder;
struct SubPass;
struct Region;
class StaticMeshComponent;
class Sampler;
class Layout;
struct FrameGraphMutableBufferId;
struct KeepAlive;
struct InlineBlock;
class ThreadDeviceLinked;
class LocalFileSystemModel;
struct SubPassData;
class StaticMesh;
class Renderable;
struct ShadowMapPass;
class Swapchain;
struct ResourceCreateInfo;
struct Semaphores;
class SpotLightComponent;
class TransformableComponent;
class ResourceFence;
class AssetLoadingContext;
struct BloomPass;
class MeshData;
class FolderFileSystemModel;
class Material;
class Window;
class DeviceMemoryAllocator;
struct CmdBuffer;
class DeviceMemory;
struct AtmospherePass;
class ComputeProgram;
class SpirVData;
class FrameGraphFrameResources;
struct ResourceUsageInfo;
struct EnclosingSphere;
struct Attribute;
class SwapchainImage;
struct BufferCreateInfo;
class CmdBufferRecorder;
struct BlurSettings;
class SkyLightComponent;
class SQLiteAssetStore;
struct Contants;
struct ScopedDevice;
class DeviceMemoryHeap;
class AABB;
class SimpleMaterialData;
class SubBufferBase;
class DebugUtils;
class AssetDependencies;
struct BoneTransform;
class BufferBase;
struct AssetData;
struct DefaultRenderer;
class DescriptorSetLayout;
struct LayoutPools;
struct RendererSettings;
struct BoneKey;
struct InlineStorage;
class PointLightComponent;
class CmdBufferData;
class Instance;
class Framebuffer;
struct SSAOPass;
struct DeviceProperties;
struct LightingPass;
class DescriptorSetAllocator;
struct SceneData;
class ImageBarrier;
class LifetimeManager;
struct FrameGraphMutableImageId;
class AccelerationStructure;
class AtmosphereComponent;
class Mapping;
struct Box;
struct GBufferPass;
struct AssetId;
class DeviceMemoryView;
struct Vertex;
class ShaderModuleBase;
struct DownsamplePass;
class ThreadLocalDevice;
class DedicatedDeviceMemoryAllocator;
class DescriptorSet;
class SQLiteFileSystemModel;
class SceneView;
struct ShadowMapSettings;
class ShaderProgram;
class GenericAssetPtr;
class FrameGraphResourcePool;
class DescriptorSetData;
struct SubMesh;
class FrameGraph;
class GraphicPipeline;
class DescriptorSetPool;
class AssetStore;
struct FrameToken;
class ImageFormat;
struct FrameGraphImageId;
struct BloomSettings;
struct SkinnedVertex;
class Queue;
class LoaderBase;
class AssetLoader;
class ImageBase;
struct ToneMappingSettings;
class FolderAssetStore;
struct ImageCopyInfo;
class DescriptorSetBase;
class SpecializationData;
class DeviceLinked;
class AnimationChannel;
class FrameGraphPassBuilder;
class AssetLoaderSystem;
struct LightingSettings;
class DeviceResources;
struct SkinWeights;
class MaterialTemplate;
struct BlurPass;
class PhysicalDevice;
class Camera;
class FrameGraphPass;
struct AttachmentData;
class Device;
struct Bone;
class EventHandler;
class SkeletonInstance;
class ImageData;
struct FreeBlock;
struct SceneRenderSubPass;
class FrameGraphDescriptorBinding;
class CmdBufferPool;
struct Monitor;
class TimeQuery;
class IBLProbe;
class Skeleton;
class SkinnedMesh;
class FrameGraphRegion;
class PushConstant;
class FileSystemModel;
struct ImageCreateInfo;
class AssetLoadingThreadPool;
class Frustum;
class Animation;
struct EmptyResource;
class AssetIdFactory;
class LoadingJob;
class InlineDescriptor;
class BufferBarrier;
class MaterialTemplateData;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkStructInitializer;
struct VkNull;
}


namespace yave::ecs {
class SparseIdSet;
class ComponentBoxBase;
class System;
struct ComponentRuntimeInfo;
class EntityIdPool;
struct ComponentsReturnPolicy;
class EntityWorld;
class Archetype;
class EntityId;
class ComponentContainerBase;
class EntityPrefab;
class IdComponents;
struct IdComponentsReturnPolicy;
struct IdReturnPolicy;
}


namespace yave::uniform {
struct PointLight;
struct Camera;
struct ShadowMapParams;
struct DirectionalLight;
struct ToneMappingParams;
struct Surfel;
struct SpotLight;
struct RayleighSky;
struct SH;
}


#endif // YAVE_UTILS_FORWARD_H